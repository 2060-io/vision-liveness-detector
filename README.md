# Liveness Detector

## Overview

Liveness Detector is a project designed to detect user liveness, leveraging a server-based architecture to process data via a Python launcher. The project integrates with the MediaPipe framework for advanced media processing capabilities.

## Features

- Provides a Python interface to launch and manage the liveness detection server.
- Includes precompiled binaries for server operation.
- Uses MediaPipe for robust and efficient media processing.

## Setup and Installation

### Prerequisites

- Python 3.6 or above
- Bazel (for building the server)
- MediaPipe (as a submodule)

### Installation

1. **Clone the repository:**

    ```bash
    git clone https://your.repo.url/liveness-detector.git
    cd liveness-detector
    ```

2. **Initialize and update submodules:**

    ```bash
    git submodule update --init --recursive
    ```

3. **Set up the project:**

    Run the setup script to copy the necessary files into the MediaPipe directory.

    ```bash
    ./setup_for_build.sh
    ```

4. **Build the server application:**

    Navigate to the MediaPipe directory and build using Bazel. 

    ```bash
    cd mediapipe
    bazel build --define MEDIAPIPE_DISABLE_GPU=1 //livenessDetectorServerApp:livenessDetectorServer
    ```

5. **Install the Python package:**

    Return to the project root and install the Python package using pip.

    ```bash
    pip install .
    ```

## Usage

To launch the liveness detector server using the Python launcher:

```bash
liveness-detector-launcher
```

## Example

The repository includes an example usage script to demonstrate how to utilize the server:

```bash
python example_run.py
```

## Contributing

Contributions are welcome! Please fork the repository and use a feature branch. Pull requests are warmly welcome.

## License

This project is licensed under the GNU AFFERO GENERAL PUBLIC LICENSE - see the [LICENSE](LICENSE) file for details.

## Contact

For issues, please open an issue via the repository's issue tracker.

