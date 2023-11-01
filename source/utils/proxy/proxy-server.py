# Proxy server for a remote data reading
# 11/1/2023
#      Author: Dmitry Ganyushin ganyushin@gmail.com
# Accepts incoming HTTP requests and transform them to sftp requests
# using pycurl or paramiko libraries

import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
import paramiko
import getpass
import pycurl
from io import BytesIO
import logging

HOST = "127.0.0.1"
PORT = 9999
logging.basicConfig(level = logging.INFO)
"""
Typical test command
curl --http0.9 http://127.0.0.1:9999/path/test.bp/md.idx -i -H "Range: bytes=0-10"
"""
buf = BytesIO()
curl = pycurl.Curl()
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
    def do_GET(self):
        logging.info("GET request, Path: %s Headers: %s\n", str(self.path), str(self.headers))
        filepath = self.path
        curl.reset()
        curl.setopt(pycurl.URL, "sftp://" + REMOTE_HOST + "/" + filepath)
        curl.setopt(pycurl.WRITEFUNCTION, buf.write)
        curl.setopt(pycurl.NOPROGRESS, 1)
        curl.setopt(pycurl.USERPWD, user + ":" + password)
        header = self.headers["Range"]
        if header:
            ranges = header.split("=")[1]
            """this is in fact ADIOS2 block. Expecting a reasonable size"""
            curl.setopt(pycurl.RANGE, ranges)
            buf.truncate(0)
            buf.seek(0)
            curl.perform()
            """send data back"""
            val = buf.getvalue()
            logging.info("sending %s bytes", str(len(val)))
            self.wfile.write(val)

            return

        header = self.headers["Content-Length"]

        if header:
            curl.setopt(curl.NOBODY, 1)
            curl.perform()
            size = curl.getinfo(curl.CONTENT_LENGTH_DOWNLOAD)
            curl.setopt(curl.NOBODY, 0)
            """send data back"""
            logging.info("sending %s", str(int(size)))
            self.wfile.write(bytes(str(int(size)), "utf-8)"))
            return

        self.wfile.write("Ok".encode("utf-8"))


def auth():
    REMOTE_HOST = input("hostname: ")
    pkey = None
    client = None

    if len(sys.argv) > 2 and sys.argv[2] == "--auth=2":
        auth_key = input("Auth key:")
        key_password = ""
        try:
            key_password = getpass.getpass()
        except Exception as error:
            logging.info('ERROR: %s', str(error))
        pkey = paramiko.RSAKey.from_private_key_file(auth_key, password=key_password)
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    user = input("Username: ")
    try:
        password = getpass.getpass()
    except Exception as error:
        logging.info('ERROR : %s', str(error))
    return (client, REMOTE_HOST, user, pkey, password)

def main_paramiko(client, REMOTE_HOST, user, pkey, password):
    global sftp
    if len(sys.argv) > 2 and sys.argv[2] == "--auth=2":
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

def main_curl(REMOTE_HOST, user, pkey, password):
    global curl
    server = HTTPServer((HOST, PORT), ADIOS_HTTP_CURL_Request)
    try:
        # Listen for requests
        logging.info("Server now serving on port %s", str(PORT))
        server.serve_forever()

    except KeyboardInterrupt:
        logging.info("Shutting down")
        server.server_close()
        logging.info("Server stopped")



if __name__ == "__main__":
    (client, REMOTE_HOST, user, pkey, password) = auth()
    if len(sys.argv) > 1 and sys.argv[1] == "--mode=paramiko":
        main_paramiko(client, REMOTE_HOST, user, pkey, password)
    elif len(sys.argv) > 1 and sys.argv[1] == "--mode=curl":
        main_curl(REMOTE_HOST, user, pkey, password)
