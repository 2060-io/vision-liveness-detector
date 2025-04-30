import cv2
import time
import argparse
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
    parser = argparse.ArgumentParser(description="Liveness detection test using a video file.")
    parser.add_argument('--video', required=True, help='Path to the video file to use as input')
    args = parser.parse_args()

    server_client = GestureServerClient(
        language="en",
        socket_path="/tmp/mysocket",
        num_gestures=2
    )

    server_client.set_string_callback(string_callback)
    server_client.set_report_alive_callback(report_alive_callback)

    frame = None

    def take_picture_wrapper(take_picture):
        if frame is not None:
            take_picture_callback(take_picture, frame)

    server_client.set_take_picture_callback(take_picture_wrapper)

    # Start the server
    if server_client.start_server():
        cap = cv2.VideoCapture(args.video)
        if not cap.isOpened():
            print(f"Error: Could not open video file '{args.video}'.")
            return

        try:
            print("Press ESC to exit (if running in an environment where keypresses are captured)")
            while True:
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0) # rewind to start
                while True:
                    ret, frame = cap.read()
                    if not ret:
                        # End of video, break inner while to loop
                        break
                    processed_frame = server_client.process_frame(frame)
                    if processed_frame is not None:
                        print("Processed a frame.")
                    # Check for ESC key (27), waitKey(1) returns key code or -1
                    if cv2.waitKey(1) & 0xFF == 27:
                        raise KeyboardInterrupt # manual exit
                    time.sleep(0.03)  # ~30 FPS

        except KeyboardInterrupt:
            print("Exiting on ESC or Ctrl+C.")
        finally:
            cap.release()
        server_client.stop_server()

if __name__ == "__main__":
    main()