import os
import platform
import subprocess
import signal
import time
import numpy as np
import json

from .unix_socket_transport import UnixSocketTransport
from .protocol_handler import ProtocolHandler

def get_server_executable_path():
    system = platform.system().lower()
    machine = platform.machine().lower()

    if system == "linux" and machine == "x86_64":
        return "./server/livenessDetectorServer"
    elif system == "windows" and machine.endswith("64"):
        return "./server/livenessDetectorServer.exe"
    elif system == "darwin" and machine == "arm64":
        return "./server/livenessDetectorServer"
    else:
        sys.exit(f"Unsupported platform: {system} {machine}")


class GestureServerClient:
    """
    Client for the liveness detector server, using a pluggable transport and protocol handler.
    """
    def __init__(
        self,
        language,
        socket_path,
        num_gestures,
        extra_gestures_paths=None,
        extra_locales_paths=None,
        gestures_list=None,
        glasses_detector_mode="OFF",
        glasses_model_path=None
    ):
        # Set up resources and server args
        self.server_executable_path = os.path.join(os.path.dirname(__file__), get_server_executable_path())
        self.model_path = os.path.join(os.path.dirname(__file__), './model/face_landmarker.task')
        self.gestures_folder_path = os.path.join(os.path.dirname(__file__), './gestures')
        self.font_path = os.path.join(os.path.dirname(__file__), './fonts/DejaVuSans.ttf')
        self.language = language
        self.socket_path = socket_path
        self.num_gestures = num_gestures

        self.extra_gestures_paths = extra_gestures_paths if extra_gestures_paths else []
        self.extra_locales_paths = extra_locales_paths if extra_locales_paths else []
        self.gestures_list = gestures_list if gestures_list else []

        self.glasses_detector_mode = glasses_detector_mode if glasses_detector_mode else "OFF"
        self.glasses_model_path = glasses_model_path or os.path.join(os.path.dirname(__file__), './model/glasses_model.onnx')

        self.server_process = None
        self.transport = None  # Will be set to UnixSocketTransport or other
        self.protocol = None   # ProtocolHandler instance

        # Optional user callback hooks
        self.string_callback = None
        self.take_picture_callback = None
        self.report_alive_callback = None

    def set_string_callback(self, callback):
        """ Set the callback function for string/JSON messages. """
        self.string_callback = callback

    def set_take_picture_callback(self, callback):
        """ Set the callback function for the takeAPicture event. """
        self.take_picture_callback = callback

    def set_report_alive_callback(self, callback):
        """ Set the callback function for the reportAlive event. """
        self.report_alive_callback = callback

    def set_font_path(self, font_path):
        """ Set the font path to be used. Use before calling start_server."""
        self.font_path = font_path

    def cleanup_socket(self):
        """ Remove the socket file if it exists (for UNIX transport). """
        try:
            if os.path.exists(self.socket_path):
                os.remove(self.socket_path)
                print(f"Removed existing socket file at {self.socket_path}.")
        except OSError as e:
            print(f"Error removing socket file: {e}")

    def start_server(self):
        """ Start the server process and establish transport/protocol connection. """
        self.cleanup_socket()

        # Compose arguments
        all_gesture_paths = [self.gestures_folder_path] + self.extra_gestures_paths
        gestures_folder_arg = ':'.join(all_gesture_paths)

        locales_paths_arg = None
        if self.extra_locales_paths:
            locales_paths_arg = ':'.join(self.extra_locales_paths)
        gestures_list_arg = None
        if self.gestures_list:
            gestures_list_arg = ':'.join(self.gestures_list)

        server_command = [
            self.server_executable_path,
            "--model_path", self.model_path,
            "--gestures_folder_path", gestures_folder_arg,
            "--language", self.language,
            "--socket_path", self.socket_path,
            "--num_gestures", str(self.num_gestures),
            "--font_path", self.font_path,
        ]
        if locales_paths_arg:
            server_command.extend(["--locales_paths", locales_paths_arg])
        if gestures_list_arg:
            server_command.extend(["--gestures_list", gestures_list_arg])
        # Glasses detector handling
        mode_str = str(self.glasses_detector_mode).strip().upper()
        if mode_str != "OFF":
            server_command.extend(["--glasses_detector", mode_str])
            if self.glasses_model_path:
                server_command.extend(["--glasses_model_path", self.glasses_model_path])
        elif self.glasses_model_path:
            # If only model path given, add it explicitly (mode is OFF)
            server_command.extend(["--glasses_detector", "OFF"])
            server_command.extend(["--glasses_model_path", self.glasses_model_path])

        print("Launching server with:", " ".join(server_command))
        self.server_process = subprocess.Popen(server_command)

        # Wait for the server to create the socket.
        start_time = time.time()
        while not os.path.exists(self.socket_path):
            if time.time() - start_time > 30:
                print("Timeout while waiting for the server to create the socket.")
                self.stop_server()
                return False
            print(f"Waiting for server socket at {self.socket_path}...")
            time.sleep(0.5)

        # --- Establish ProtocolHandler over the transport ---
        self.transport = UnixSocketTransport(self.socket_path)
        try:
            self.transport.connect()
            print("Connected to server")
            self.protocol = ProtocolHandler(self.transport)
            return True
        except Exception as e:
            print(f"Error connecting to server: {e}")
            self.stop_server()
            return False

    def set_overwrite_text(self, text):
        """ Send a command to the server to set the overwrite text. """
        if not self.protocol:
            raise RuntimeError("Server not started or connection failed.")

        self.protocol.send_config_json({
            "action": "set",
            "variable": "overwrite_text",
            "value": text
        })

    def set_warning_message(self, text):
        """ Send a command to the server to set the warning message. """
        if not self.protocol:
            raise RuntimeError("Server not started or connection failed.")

        self.protocol.send_config_json({
            "action": "set",
            "variable": "warning_message",
            "value": text
        })

    def process_frame(self, frame):
        """
        Send a frame to the server and receive the processed frame (plus any JSON events).
        Returns a processed frame np.ndarray or None on failure.
        """
        if self.protocol is None:
            raise RuntimeError("Server not started or connection failed.")

        self.protocol.send_image_request(frame)

        processed_frame = None
        # Loop for responses: the server may send one or more JSON (0x02) before the processed image (0x01)
        while True:
            msg_type, data = self.protocol.recv_message()
            if msg_type == 0x02:
                self.handle_json_response(data)
            elif msg_type == 0x01:
                processed_frame = data
                break
            elif msg_type is None:
                break
        return processed_frame

    def handle_json_response(self, string_data):
        """
        Handle the JSON response and call appropriate callbacks, if set.
        """
        try:
            json_data = json.loads(string_data)
            if self.string_callback:
                self.string_callback(string_data)
            if 'takeAPicture' in json_data and self.take_picture_callback:
                self.take_picture_callback(json_data['takeAPicture'])
            if 'reportAlive' in json_data and self.report_alive_callback:
                self.report_alive_callback(json_data['reportAlive'])
        except json.JSONDecodeError:
            print(f"Failed to decode JSON string: {string_data}")

    def stop_server(self):
        """ Stop the server process and clean up transport. """
        if self.server_process:
            self.server_process.send_signal(signal.SIGTERM)
            self.server_process.wait()
            self.server_process = None
        if self.protocol:
            self.protocol = None
        if self.transport:
            self.transport.close()
            self.transport = None
        self.cleanup_socket()