import struct
import json
import numpy as np

class ProtocolHandler:
    def __init__(self, transport):
        self.transport = transport

    def send_json(self, data: dict):
        payload = json.dumps(data).encode('utf-8')
        self.transport.sendall(b'\x02')
        self.transport.sendall(struct.pack('>I', len(payload)))
        self.transport.sendall(payload)

    def send_image(self, arr: np.ndarray):
        # arr must be contiguous, dtype=uint8, shape=(h,w,c)
        rows, cols, channels = arr.shape
        frame_bytes = arr.tobytes()
        self.transport.sendall(b'\x01')
        self.transport.sendall(struct.pack('>I', len(frame_bytes)))
        self.transport.sendall(struct.pack('>I', rows))
        self.transport.sendall(struct.pack('>I', cols))
        self.transport.sendall(frame_bytes)

    def recv_message(self):
        # Returns (function_id, payload_bytes)
        fid = self.transport.recv(1)
        if not fid:
            return None, None

        fid = fid[0]
        if fid == 0x01:
            size = struct.unpack('>I', self.transport.recv(4))[0]
            rows = struct.unpack('>I', self.transport.recv(4))[0]
            cols = struct.unpack('>I', self.transport.recv(4))[0]
            img_bytes = self._recv_all(size)
            img = np.frombuffer(img_bytes, dtype=np.uint8).reshape((rows, cols, 3))
            return 0x01, img
        elif fid == 0x02:
            string_size = struct.unpack('>I', self.transport.recv(4))[0]
            string_bytes = self._recv_all(string_size)
            s = string_bytes.decode('utf-8')
            return 0x02, s
        else:
            return fid, None

    def send_image_request(self, frame: np.ndarray):
        # Client-to-server request for image processing
        rows, cols, channels = frame.shape
        frame_bytes = frame.tobytes()
        self.transport.sendall(b'\x01')
        self.transport.sendall(struct.pack('>I', len(frame_bytes)))
        self.transport.sendall(struct.pack('>I', rows))
        self.transport.sendall(struct.pack('>I', cols))
        self.transport.sendall(frame_bytes)

    def send_config_json(self, data: dict):
        # Client-to-server config request
        payload = json.dumps(data).encode('utf-8')
        self.transport.sendall(b'\x02')
        self.transport.sendall(struct.pack('>I', len(payload)))
        self.transport.sendall(payload)

    def _recv_all(self, nbytes):
        buf = b''
        while len(buf) < nbytes:
            part = self.transport.recv(nbytes - len(buf))
            if not part:
                raise ConnectionError('Unexpected disconnect')
            buf += part
        return buf