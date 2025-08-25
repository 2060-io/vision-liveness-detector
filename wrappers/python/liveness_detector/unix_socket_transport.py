import socket
import os
from .transport import Transport

class UnixSocketTransport(Transport):
    def __init__(self, socket_path):
        self.socket_path = socket_path
        self.sock = None

    def connect(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def sendall(self, data: bytes):
        self.sock.sendall(data)

    def recv(self, nbytes: int) -> bytes:
        return self.sock.recv(nbytes)

    def is_connected(self) -> bool:
        return self.sock is not None