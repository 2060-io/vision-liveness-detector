# Gesture-based Facial Liveness Detector

by [2060.io](https://2060.io) — Secure, customizable, open-source liveness detection with randomized facial gestures and real-time computer vision.

---

![Demo Screenshot](https://raw.githubusercontent.com/2060-io/vision-liveness-detector/refs/heads/main/docs/Screenshot_sm.png)

---

## Project Overview

The Liveness Detector enables reliable human presence verification using randomized facial gesture challenges.  
Unlike traditional face checks, this system prompts users for live gestures (blink, smile, head turn, etc.), fighting replay and spoof attacks.

- Fast: C++/MediaPipe core, controlled from Python.
- Flexible: Add gestures/locales with JSON.
- User-friendly: Minimal Python code, easy to customize.
- Extensible: For both production users and contributors.

---

## For Python Users — Install & Use

### 1. Install via PyPI

> **✨ Now on [PyPI](https://pypi.org/project/liveness-detector/)! ✨**  
> You can install the liveness detector with:
>
> ```bash
> pip install liveness-detector
> ```
>
> _Note: The package is currently distributed with Linux wheels only; Windows/macOS support is coming soon!_

---

### 2. Minimal Video Usage Example (Webcam Integration)

This script captures video frames from the webcam, processes them, and displays real-time liveness feedback:

```python
import cv2
from liveness_detector.server_launcher import GestureServerClient

def string_callback(message):
    print(f"Message: {message}")

def report_alive_callback(alive):
    print(f"Liveness result: {'ALIVE' if alive else 'NOT ALIVE'}")

def main():
    server_client = GestureServerClient(
        language='en',
        socket_path='/tmp/liveness_socket',
        num_gestures=2
    )
    server_client.set_string_callback(string_callback)
    server_client.set_report_alive_callback(report_alive_callback)

    if server_client.start_server():
        cap = cv2.VideoCapture(0)  # Start webcam
        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    break

                processed = server_client.process_frame(frame)
                if processed is not None:
                    cv2.imshow("Liveness Detector", processed)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        finally:
            cap.release()
            cv2.destroyAllWindows()
            server_client.stop_server()

if __name__ == "__main__":
    main()
```
You can further use callbacks to save images or trigger custom actions.

---

## Custom Gestures and Translations

### Add Gestures / Languages at Runtime

You can pass extra gesture directories or translation files when constructing the Python `GestureServerClient`:

```python
server_client = GestureServerClient(
    language='es',
    socket_path='/tmp/liveness_socket',
    num_gestures=2,
    extra_gestures_paths=['/path/to/my_custom_gestures'],
    extra_locales_paths=['/path/to/my_custom_locales'],
    gestures_list=['blink', 'openCloseMouth']
)
```

- `extra_gestures_paths`: List of folders with additional gesture JSONs.
- `extra_locales_paths`: List of folders with more translation JSONs.
- `gestures_list`: (Optional) List of gesture names to permit for this session.

Example custom gesture:
```json
{
    "gestureId": "blink",
    "label": "Blink your eyes",
    "signal_key": "eyeBlinkLeft",
    "instructions": [
        {"move_to_next_type": "lower", "value": 0.35, "reset": {"type": "timeout_after_ms", "value": 10000}}
    ]
}
```

Example custom locale (for Spanish):
```json
{
    "gestures": {
        "blink": {"label": "Parpadea"}
    },
    "warning": {
        "wrong_face_width_message": "El ancho de la cara no es correcto."
    }
}
```

---

## Features

- Challenge-Based Liveness: Random gestures defeat spoofing and replay.
- Real-Time Response: MediaPipe and C++ for instant feedback, smooth overlays.
- Pluggable Gestures and Languages: Just add JSON, no code required.
- Python-First: Drop-in camera pipelines with only a few lines.
- Extensible for Power Users: Add pipeline steps, new models, or UI feedback.

---

## Security and Privacy

- All processing local by default.
- Open source: audit and improve as you wish.
- Randomization ensures every session is unique.

---

## License

This project is licensed under the [GNU Affero General Public License](LICENSE).

---

## Contact and Support

- [GitHub Issues](https://github.com/2060-io/vision-liveness-detector/issues)
- Learn more: [https://2060.io](https://2060.io)