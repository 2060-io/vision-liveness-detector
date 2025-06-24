import cv2
import time
from liveness_detector.server_launcher import GestureServerClient

def string_callback(message):
    print(f"Callback received message: {message}")

def take_picture_callback(take_picture, frame):
    if take_picture:
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        filename = f"captured_frame_{timestamp}.png"
        cv2.imwrite(filename, frame)
        print(f"Image saved as {filename}")

def report_alive_callback(alive):
    print(f"Callback: The Person is {'alive' if alive else 'not alive'}.")

def main():
    # Setup the class with the necessary parameters
    server_client = GestureServerClient(
        language="es",
        socket_path="/tmp/mysocket",
        num_gestures=2
    )

    # IMPORTANT: set_warning_message should exist in GestureServerClient!
    # If not, ensure you implement/proxy it properly.

    # Set the callback functions
    server_client.set_string_callback(string_callback)
    server_client.set_report_alive_callback(report_alive_callback)

    frame = None  # Initialize frame

    # Wrapper for take_picture_callback to include frame
    def take_picture_wrapper(take_picture):
        if frame is not None:
            take_picture_callback(take_picture, frame)

    server_client.set_take_picture_callback(take_picture_wrapper)

    # Start the server
    if server_client.start_server():
        cap = cv2.VideoCapture(0)  # Use webcam for live video capture

        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    break
                processed_frame = server_client.process_frame(frame)
                if processed_frame is not None:
                    cv2.imshow('Processed Frame', processed_frame)

                key = cv2.waitKey(1) & 0xFF

                if key == ord('q'):
                    break
                elif key == ord('o'):
                    # Overwrite text message
                    overwrite_text = "This is the Overwrite text message in multiple lines, so I have to write a very big text to have multiple lines"
                    server_client.set_overwrite_text(overwrite_text)
                elif key == ord('w'):
                    # Set warning message
                    warning_text = "This is the Warning text message in multiple lines, so I have to write a very big text to have multiple lines"
                    server_client.set_warning_message(warning_text)
                elif key == ord('W'):
                    # Set warning message
                    warning_text = ""
                    server_client.set_warning_message(warning_text)
                elif key == ord('O'):
                    # Overwrite text message
                    overwrite_text = ""
                    server_client.set_overwrite_text(overwrite_text)
                
        finally:
            cap.release()
            cv2.destroyAllWindows()

        # Stop the server
        server_client.stop_server()

if __name__ == "__main__":
    main()