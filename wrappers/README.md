# Liveness Detector

## Overview

Liveness Detector is a project designed to detect user liveness, leveraging a server-based architecture to process data via a Python launcher. The project integrates with the MediaPipe framework for advanced media processing capabilities.

## Features

- Provides a Python interface to launch and manage the liveness detection server.
- Includes precompiled binaries for server operation.
- Uses MediaPipe for robust and efficient media processing.

## Setup and Installation

### Prerequisites

- Python 3.6 or above (up to 3.12)
- Bazel (for building the server)
- OpenCV
- MediaPipe (as a submodule)


> *NOTE*: if you run into troubles when linking OpenCV headers, try adding a symbolic link to opencv4 with opencv2, i.e.:
> sudo ln -s /usr/include/opencv4/opencv2 /usr/include/opencv2                                      
> 
### Installation

1. **Clone the repository:**

    ```bash
    git clone https://your.repo.url/liveness-detector.git
    cd liveness-detector
    ```

2. **Set up the project:**

    Run the setup script to copy the necessary files into the MediaPipe directory.

    ```bash
    ./setup_for_build.sh
    ```

3. **Build the server application:**

    Navigate to the MediaPipe directory and build using Bazel. 

    ```bash
    cd build/mediapipe
    bazel build --define MEDIAPIPE_DISABLE_GPU=1 //livenessDetectorServerApp:livenessDetectorServer
    ```

4. **Install the Python package:**

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

