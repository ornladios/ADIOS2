# Proxy server for a remote data reading
# 11/1/2023
#      Author: Dmitry Ganyushin ganyushin@gmail.com
# Accepts incoming HTTP requests and transform them to sftp requests
# using pycurl or paramiko libraries

from http.server import HTTPServer, BaseHTTPRequestHandler
import logging
import os

HOST = "127.0.0.1"
PORT = 9999
logging.basicConfig(level=logging.INFO)
"""
"""

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
    main_http()

