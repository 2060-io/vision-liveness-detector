# Liveness Detector

## Overview

Liveness Detector is a project designed to detect user liveness, leveraging a server-based architecture to process data via a Python launcher. The project integrates with the MediaPipe framework for advanced media processing capabilities.

## Features

- Provides a Python interface to launch and manage the liveness detection server.
- Includes precompiled binaries for server operation.
- Uses MediaPipe for robust and efficient media processing.

## Setup and Installation

### Requirements

- Docker with BUILDKIT enabled must be installed and running on your system.
- Python 3.6 or above (up to 3.12) for using the generated wheel.

### Build

To build the Python wheel, simply run:

```bash
./build.sh
```

This will use the provided `Dockerfile.manylinux_2_28_x86_64` to build the project inside a Docker container. After the build completes, the Python wheel file will be generated at:

```
wrappers/python/dist/liveness_detector-0.1.0-py3-none-manylinux2014_x86_64.whl
```

### Publish

To publish the generated wheel to PyPI, use:

```bash
./publish.sh
```

This script uses `twine` to upload all files in `wrappers/python/dist/` to the Python Package Index (PyPI). You must have a valid PyPI account and credentials configured (typically via a `~/.pypirc` file or environment variables) before running the script.

After publishing, anyone can install the package directly from PyPI using:

```bash
pip install liveness_detector
```

## Usage

After building, you can install the Python package in two ways:

- **Install the locally built wheel:**

    ```bash
    pip install wrappers/python/dist/liveness_detector-0.1.0-py3-none-manylinux2014_x86_64.whl
    ```

- **Install from PyPI (after publishing):**

    ```bash
    pip install liveness_detector
    ```

You can then launch the liveness detector server using the Python launcher:

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
