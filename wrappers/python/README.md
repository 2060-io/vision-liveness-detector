# Gesture-based Facial Liveness Detector

by [2060.io](https://2060.io) — Secure, customizable, open-source liveness detection with randomized facial gestures and real-time computer vision.

---

![Demo Screenshot](docs/Screenshot_sm.png)

---

## About 2060.io

2060.io builds open-source tools for creating rich, decentralized, chat-powered services.
We enable next-generation authentication, messaging, and Verifiable Credentials workflows—combining text, images, video, voice, and biometric authentication, all underpinned by privacy, interoperability, and the power of self-sovereign identity.

- Verifiable Credentials & Authentic Data  
    Our open-source tech lets you issue, verify, and use trustworthy credentials that embed OpenID for Verifiable Credentials, and DIDComm-based auditable chat messaging.
    Easily compose anonymous, decentralized, and interoperable services using our [service agent](https://github.com/2060-io/2060-service-agent) (built atop OpenWallet Credo TS).
    Deliver and verify anything—from identity cards and badges to graduation certificates and more.

- Biometric Authentication you can trust  
    This Liveness Detector verifies a real human presence and compares faces against the embedded photos in verifiable credentials, increasing security for all decentralized applications.

---

## Project Overview

The Liveness Detector enables reliable human presence verification using randomized facial gesture challenges.  
Unlike traditional face checks, this system prompts users for live gestures (blink, smile, head turn, etc.), fighting replay and spoof attacks.

- Fast: C++/MediaPipe core, controlled from Python.
- Flexible: Add gestures/locales with JSON.
- User-friendly: Minimal Python code, easy to customize.
- Extensible: For both production users and contributors.

---

## Who Is This For?

- Python Users: Want a plug-and-play library via pip to add liveness to their app or workflow (including support for custom gestures and translations).
- Contributors/Developers: Want to build from source, enhance or hack the C++ backend, or submit improvements.

Both user types can add gestures and languages—see below.

---

## For Python Users — Install & Use

Note: The PyPI/package wheel currently works on Linux only; Windows/macOS support coming soon.

### 1. Install via pip (Linux only for now)
```bash
pip install liveness_detector
```
Or, after building from source:  
```bash
pip install wrappers/python/dist/liveness_detector-*.whl
```

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
You can further use callbacks (see [example_run.py](wrappers/python/tests/example_run.py)) to save images or trigger custom actions.

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

- extra_gestures_paths: List of folders with additional gesture JSONs.
- extra_locales_paths: List of folders with more translation JSONs.
- gestures_list: (Optional) List of gesture names to permit for this session.

These correspond internally to `--gestures_folder_path`, `--locales_paths`, and `--gestures_list` parameters for the C++ server.

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
See [src/livenessDetectorServerApp/gestures/blink.json](src/livenessDetectorServerApp/gestures/blink.json) for a reference.

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
Just place your custom JSON files in extra directories and reference them as shown above.

---

## Architecture

```text
Your Python App
    │
    ▼
[Liveness Detector Python Wrapper]
    │
    ▼   (Unix Socket IPC: frames, commands, responses)
[C++ Liveness Detector Server]
    │
    ▼
[MediaPipe / Face Landmarker]
```
- The Python wrapper launches and talks to the server transparently.
- Custom gestures/locales are picked up as configured.
- Responses, instructions, overlays, and result events all flow through the Python API.

---

## Features

- Challenge-Based Liveness: Random gestures defeat spoofing and replay.
- Real-Time Response: MediaPipe and C++ for instant feedback, smooth overlays.
- Pluggable Gestures and Languages: Just add JSON, no code required.
- Python-First: Drop-in camera pipelines with only a few lines.
- Extensible for Power Users: Add pipeline steps, new models, or UI feedback.

---

## Example Use Cases

- Decentralized KYC/client onboarding: Stop fake users and verify real human presence before issuing credentials.
- Remote proctoring for online exams: Ensure test-taker presence.
- Digital signature and credential session verification: Make sure the signer is truly present and matches credential data.
- AR/VR "are you present?" checks: Detect real humans in immersive environments.
- Fraud prevention and access control: Augment authentication and reduce risk of replay attacks.

---

## For Contributors and Developers — Build and Extend

### 1. Build From Source

```bash
git clone https://github.com/2060-io/vision-liveness-detector.git
cd vision-liveness-detector
./build.sh
pip install wrappers/python/dist/liveness_detector-*.whl
```
Requires Docker and BUILDKIT on Linux.

### 2. Direct C++ Server Usage

You can run the C++ server binary standalone for integration with any client:

```bash
livenessDetectorServer \
    --model_path path/to/model \
    --gestures_folder_path path/to/gestures \
    --language en \
    --socket_path /tmp/liveness.sock \
    --num_gestures 2 \
    --font_path path/to/DejaVuSans.ttf
    # --locales_paths and --gestures_list also supported
```

### 3. How to Extend Internals

- Modify gesture logic, detection thresholds, draw overlays, etc. in C++.
- Contribute on GitHub; see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Security and Privacy

- All processing local by default.
- Open source: audit and improve as you wish.
- Randomization ensures every session is unique.

---

## Contributing to the 2060.io Ecosystem

Pull requests, issues, and gesture/locale files are welcome.
See [CONTRIBUTING.md](CONTRIBUTING.md) for developer workflow or reach out to learn more about our community.
Want to build Verifiable Credential and DIDComm services? Explore our agent framework: [2060-service-agent](https://github.com/2060-io/2060-service-agent).

---

## License

This project is licensed under the [GNU Affero General Public License](LICENSE).

---

## Contact and Support

- [GitHub Issues](https://github.com/2060-io/vision-liveness-detector/issues)
- Learn more: [https://2060.io](https://2060.io)
