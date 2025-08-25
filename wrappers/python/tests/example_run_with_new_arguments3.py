import cv2
import time
from liveness_detector.server_launcher import GestureServerClient
import queue

frame_queue = queue.Queue(maxsize=1)  # Only keep the latest frame

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

def image_callback(processed_frame):
    try:
        # Put the processed_frame into the queue, overwriting any previous
        if not frame_queue.empty():
            try:
                frame_queue.get_nowait()
            except queue.Empty:
                pass
        frame_queue.put_nowait(processed_frame)
    except queue.Full:
        pass  # Drop frame if queue is full

def main():
    # Setup the class with the necessary parameters
    server_client = GestureServerClient(
        language="en",
        socket_path="/tmp/mysocket",
        num_gestures=3,
        gestures_list=["blink", "smile", "openCloseMouth"],
        glasses_detector_mode="WARNING_ONLY",
    )

    # Set the callback functions
    server_client.set_string_callback(string_callback)
    server_client.set_report_alive_callback(report_alive_callback)
    server_client.set_image_callback(image_callback)
    server_client.set_take_picture_callback(take_picture_callback)  # The frame will be provided by the async server

    # Start the server
    if server_client.start_server():
        cap = cv2.VideoCapture(0)  # Use webcam for live video capture

        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    break
                # Async: just send the frame, don't expect a return value
                server_client.process_frame(frame)

                # Try to display the latest processed frame
                try:
                    processed_frame = frame_queue.get_nowait()
                    cv2.imshow('Processed Frame', processed_frame)
                except queue.Empty:
                    pass

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        finally:
            cap.release()
            cv2.destroyAllWindows()
            # Stop the server
            server_client.stop_server()

if __name__ == "__main__":
    main()