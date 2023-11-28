# Proxy server for a remote data reading
# 11/1/2023
#      Author: Dmitry Ganyushin ganyushin@gmail.com
# Accepts incoming HTTP requests and transform them to sftp requests
# using pycurl or paramiko libraries

from functools import partial
from http.server import HTTPServer, BaseHTTPRequestHandler
import paramiko
import getpass
import pycurl
from io import BytesIO
import logging
import argparse
import os

HOST = "127.0.0.1"
PORT = 9999
logging.basicConfig(level=logging.INFO)
"""
Typical test command
curl --http0.9 http://127.0.0.1:9999/path/test.bp/md.idx -i -H "Range: bytes=0-10"
"""


def setup_args():
    parser = argparse.ArgumentParser('A proxy server for remote reading. python proxy ')
    parser.add_argument(
        "--mode", "-m", help="Connection method", default="curl"
    )
    parser.add_argument(
        "--port", "-p", help="port number", default=9999
    )
    parser.add_argument(
        "--auth", "-a", help="authentication mode password or key", default=""
    )
    return parser.parse_args()


class FastTransport(paramiko.Transport):

    def __init__(self, sock):
        super(FastTransport, self).__init__(sock)
        self.window_size = 2147483647
        self.packetizer.REKEY_BYTES = pow(2, 40)
        self.packetizer.REKEY_PACKETS = pow(2, 40)


class ADIOS_HTTP_PARAMIKO_Request(BaseHTTPRequestHandler):
    def do_GET(self):
        logging.info("GET request, Path: %s Headers: %s\n", str(self.path), str(self.headers))
        filepath = self.path
        remote_file = sftp.file(filepath, 'r')
        remote_file.prefetch()
        header = self.headers["Range"]
        if header:
            ranges = header.split("=")[1].split("-")
            start_byte = int(ranges[0])
            end_byte = int(ranges[1])

            block_size = end_byte - start_byte + 1
            remote_file.seek(start_byte)
            """this is in fact ADIOS2 block. Expecting a reasonable size"""
            data = remote_file.read(block_size)
            """send data back"""
            logging.info("sending %s bytes", str(len(data)))
            self.wfile.write(data)
            return

        header = self.headers["Content-Length"]

        if header:
            data = remote_file.stat()

            """send data back"""
            logging.info("sending size %s bytes", str(data.st_size))
            self.wfile.write(bytes(str(int(data.st_size)), "utf-8)"))
            return

        self.wfile.write("Ok".encode("utf-8"))


class ADIOS_HTTP_CURL_Request(BaseHTTPRequestHandler):
    buf = BytesIO()
    curl = pycurl.Curl()

    def __init__(self, REMOTE_HOST, user, password, *args, **kwargs):
        self.REMOTE_HOST = REMOTE_HOST
        self.user = user
        self.password = password
        super().__init__(*args, **kwargs)

        # trying to connect
        self.curl.setopt(pycurl.URL, "sftp://" + self.REMOTE_HOST)
        self.curl.setopt(pycurl.WRITEFUNCTION, self.buf.write)
        self.curl.setopt(pycurl.NOPROGRESS, 1)
        self.curl.setopt(pycurl.USERPWD, user + ":" + password)
        self.curl.setopt(self.curl.NOBODY, 1)
        try:
            self.curl.perform()
        except Exception as e:
            logging.info("connection error %s", str(e))
            exit(1)

        self.curl.setopt(pycurl.NOBODY, 0)

    def do_GET(self):
        logging.info("GET request, Path: %s Headers: %s\n", str(self.path), str(self.headers))
        filepath = self.path
        self.curl.reset()
        self.curl.setopt(pycurl.URL, "sftp://" + REMOTE_HOST + "/" + filepath)
        self.curl.setopt(pycurl.WRITEFUNCTION, self.buf.write)
        self.curl.setopt(pycurl.NOPROGRESS, 1)
        self.curl.setopt(pycurl.USERPWD, user + ":" + password)
        header = self.headers["Range"]
        if header:
            ranges = header.split("=")[1]
            """this is in fact ADIOS2 block. Expecting a reasonable size"""
            self.curl.setopt(pycurl.RANGE, ranges)
            self.buf.truncate(0)
            self.buf.seek(0)
            try:
                self.curl.perform()
            except Exception as e:
                logging.info("connection error %s ", str(e))
                exit(1)
            """send data back"""
            val = self.buf.getvalue()
            logging.info("sending %s bytes", str(len(val)))
            self.wfile.write(val)

            return

        header = self.headers["Content-Length"]

        if header:
            self.curl.setopt(self.curl.NOBODY, 1)
            try:
                self.curl.perform()
            except Exception as e:
                logging.info("connection error %s", str(e))
                exit(1)
            size = self.curl.getinfo(self.curl.CONTENT_LENGTH_DOWNLOAD)
            self.curl.setopt(self.curl.NOBODY, 0)
            """send data back"""
            logging.info("sending %s", str(int(size)))
            self.wfile.write(bytes(str(int(size)), "utf-8)"))
            return

        self.wfile.write("Ok".encode("utf-8"))


class ADIOS_HTTP_Request(BaseHTTPRequestHandler):
    files = {}

    def do_GET(self):
        logging.info("GET request, Path: %s Headers: %s\n", str(self.path), str(self.headers))
        filepath = self.path
        if filepath in self.files:
            h = self.files[filepath]
        else:
            h = open(filepath, "rb")
            self.files[filepath] = h
        # TODO close files by exit

        header = self.headers["Range"]
        if header:
            ranges = header.split("=")[1].split("-")
            start_byte = int(ranges[0])
            end_byte = int(ranges[1])

            block_size = end_byte - start_byte + 1
            h.seek(start_byte)
            """this is in fact ADIOS2 block. Expecting a reasonable size"""
            data = h.read(block_size)
            """send data back"""
            logging.info("sending %s bytes", str(len(data)))
            self.wfile.write(data)
            return

        header = self.headers["Content-Length"]

        if header:
            data = os.stat(filepath)
            """send data back"""
            logging.info("sending size %s bytes", str(data.st_size))
            self.wfile.write(bytes(str(int(data.st_size)), "utf-8)"))
            return

        self.wfile.write("Ok".encode("utf-8"))


def auth(args):
    global PORT
    PORT = int(args.port)
    REMOTE_HOST = input("hostname: ")
    pkey = None
    client = None

    if args.auth == "key":
        auth_key = input("Auth key:")
        key_password = ""
        try:
            key_password = getpass.getpass()
        except Exception as e:
            logging.info('ERROR: %s', str(e))
        pkey = paramiko.RSAKey.from_private_key_file(auth_key, password=key_password)
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    user = input("Username: ")
    try:
        password = getpass.getpass()
    except Exception as e:
        logging.info('ERROR : %s', str(e))
    return (client, REMOTE_HOST, user, pkey, password)


def main_paramiko(client, REMOTE_HOST, user, pkey, password):
    global sftp
    if pkey is not None:
        logging.info("connecting ...")
        client.connect(hostname=REMOTE_HOST, username=user, pkey=pkey, password=password)
        logging.info("connected")
        sftp = client.open_sftp()
    else:
        transport = FastTransport((REMOTE_HOST, 22))
        transport.connect(None, user, password)
        sftp = paramiko.SFTPClient.from_transport(transport)
    server = HTTPServer((HOST, PORT), ADIOS_HTTP_PARAMIKO_Request)
    try:
        # Listen for requests
        logging.info("Server now serving ...")
        server.serve_forever()

    except KeyboardInterrupt:
        logging.info("Shutting down")
        server.server_close()
        logging.info("Server stopped")
        exit(0)


def main_curl(REMOTE_HOST, user, pkey, password):
    handler = partial(ADIOS_HTTP_CURL_Request, REMOTE_HOST, user, password)
    server = HTTPServer((HOST, PORT), handler)
    try:
        # Listen for requests
        logging.info("Server now serving on port %s", str(PORT))
        server.serve_forever()

    except KeyboardInterrupt:
        logging.info("Shutting down")
        server.server_close()
        logging.info("Server stopped")
        exit(0)


def main_http():
    server = HTTPServer((HOST, PORT), ADIOS_HTTP_Request)
    try:
        # Listen for requests
        logging.info("Server now serving on port %s", str(PORT))
        server.serve_forever()

    except KeyboardInterrupt:
        logging.info("Shutting down")
        server.server_close()
        logging.info("Server stopped")
        exit(0)


if __name__ == "__main__":
    args = setup_args()
    if args.mode == "http":
        main_http()

    (client, REMOTE_HOST, user, pkey, password) = auth(args)
    if args.mode == "paramiko":
        main_paramiko(client, REMOTE_HOST, user, pkey, password)
    elif args.mode == "curl":
        main_curl(REMOTE_HOST, user, pkey, password)
    else:
        logging.error("mode can be curl or paramiko")
