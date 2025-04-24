import socket
import os
import platform
import subprocess
import signal
import time
import numpy as np
import json


def get_server_executable_path():
    system = platform.system().lower()
    machine = platform.machine().lower()

    if system == "linux" and machine == "x86_64":
        return "./server/livenessDetectorServer"
    elif system == "windows" and machine.endswith("64"):
        return "./server/livenessDetectorServer.exe"  # Adjust the directory names as necessary
    elif system == "darwin" and machine == "arm64":
        return "./server/livenessDetectorServer"
    else:
        sys.exit(f"Unsupported platform: {system} {machine}")

class GestureServerClient:
    def __init__(self, language, socket_path, num_gestures):
        self.server_executable_path = os.path.join(os.path.dirname(__file__), get_server_executable_path())
        self.model_path = os.path.join(os.path.dirname(__file__),'./model/face_landmarker.task')
        self.gestures_folder_path = os.path.join(os.path.dirname(__file__),'./gestures')
        self.language = language
        self.socket_path = socket_path
        self.num_gestures = num_gestures
        self.server_process = None
        self.client_socket = None
        self.string_callback = None
        self.take_picture_callback = None
        self.report_alive_callback = None

    def set_string_callback(self, callback):
        """ Set the callback function for string messages. """
        self.string_callback = callback

    def set_take_picture_callback(self, callback):
        """ Set the callback function for the takeAPicture event. """
        self.take_picture_callback = callback

    def set_report_alive_callback(self, callback):
        """ Set the callback function for the reportAlive event. """
        self.report_alive_callback = callback

    def cleanup_socket(self):
        """ Remove the socket file if it exists. """
        try:
            if os.path.exists(self.socket_path):
                os.remove(self.socket_path)
                print(f"Removed existing socket file at {self.socket_path}.")
        except OSError as e:
            print(f"Error removing socket file: {e}")

    def start_server(self):
        """ Start the server process. """
        self.cleanup_socket()

        server_command = [
            self.server_executable_path,
            self.model_path,
            self.gestures_folder_path,
            self.language,
            self.socket_path,
            str(self.num_gestures)
        ]

        print("Launching server...")
        self.server_process = subprocess.Popen(server_command)

        start_time = time.time()
        while not os.path.exists(self.socket_path):
            if time.time() - start_time > 30:
                print("Timeout while waiting for the server to create the socket.")
                self.stop_server()
                return False

            print(f"Waiting for server socket at {self.socket_path}...")
            time.sleep(0.5)

        self.client_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            self.client_socket.connect(self.socket_path)
            print("Connected to server")
            return True
        except Exception as e:
            print(f"Error connecting to server: {e}")
            self.stop_server()
            return False

    def process_frame(self, frame):
        """ Send a frame to the server and receive the processed frame. """
        if self.client_socket is None:
            raise RuntimeError("Server not started or connection failed.")

        rows, cols, channels = frame.shape
        frame_size = rows * cols * channels

        function_id = (0x01).to_bytes(1, byteorder='big')

        try:
            self.client_socket.sendall(function_id)
            self.client_socket.sendall(frame_size.to_bytes(4, byteorder='big'))
            self.client_socket.sendall(rows.to_bytes(4, byteorder='big'))
            self.client_socket.sendall(cols.to_bytes(4, byteorder='big'))
            frame_bytes = frame.tobytes()
            self.client_socket.sendall(frame_bytes)

            while True:
                response_function_id_data = self.client_socket.recv(1)
                if not response_function_id_data:
                    print("No response from server (function ID), quitting")
                    return None

                response_function_id = int.from_bytes(response_function_id_data, byteorder='big')

                if response_function_id == 0x02:
                    string_size_data = self.client_socket.recv(4)
                    string_size = int.from_bytes(string_size_data, byteorder='big')
                    string_data = self.client_socket.recv(string_size).decode('utf-8')

                    # Process the JSON response
                    self.handle_json_response(string_data)

                elif response_function_id == 0x01:
                    received_size_data = self.client_socket.recv(4)
                    processed_size = int.from_bytes(received_size_data, byteorder='big')

                    processed_rows_data = self.client_socket.recv(4)
                    processed_cols_data = self.client_socket.recv(4)
                    processed_rows = int.from_bytes(processed_rows_data, byteorder='big')
                    processed_cols = int.from_bytes(processed_cols_data, byteorder='big')

                    processed_frame_bytes = b''
                    while len(processed_frame_bytes) < processed_size:
                        packet = self.client_socket.recv(min(processed_size - len(processed_frame_bytes), 1024 * 1024))
                        if not packet:
                            print("Connection closed by server")
                            return None
                        processed_frame_bytes += packet

                    processed_frame = np.frombuffer(processed_frame_bytes, dtype=np.uint8).reshape((processed_rows, processed_cols, channels))
                    return processed_frame
        except Exception as e:
            print(f"Error processing frame: {e}")
            return None

    def handle_json_response(self, string_data):
        """ Handle the JSON response and call appropriate callbacks. """
        try:
            json_data = json.loads(string_data)
            if self.string_callback:
                self.string_callback(string_data)
            if 'takeAPicture' in json_data and self.take_picture_callback:
                self.take_picture_callback(json_data['takeAPicture'])
            if 'reportAlive' in json_data and self.report_alive_callback:
                self.report_alive_callback(json_data['reportAlive'])
        except json.JSONDecodeError as e:
            print(f"Failed to decode JSON string: {string_data}")

    def stop_server(self):
        """ Stop the server process and clean up. """
        if self.server_process:
            self.server_process.send_signal(signal.SIGTERM)
            self.server_process.wait()
            self.server_process = None
        if self.client_socket:
            self.client_socket.close()
            self.client_socket = None
        self.cleanup_socket()