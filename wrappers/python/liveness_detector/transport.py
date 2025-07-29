import abc

class Transport(abc.ABC):
    @abc.abstractmethod
    def connect(self):
        pass

    @abc.abstractmethod
    def close(self):
        pass

    @abc.abstractmethod
    def sendall(self, data: bytes):
        pass

    @abc.abstractmethod
    def recv(self, nbytes: int) -> bytes:
        pass

    @abc.abstractmethod
    def is_connected(self) -> bool:
        pass