#!/bin/bash

# Ensure the script stops if any command fails
set -e

DOCKER_BUILDKIT=1 docker build -f Dockerfile.manylinux_2_28_x86_64 -t mediapipe_liveness_detector:latest --build-arg PYTHON_BIN=/opt/python/cp312-cp312/bin/python3.12 .
docker create -ti --name liveness_detector_pip_package_container mediapipe_liveness_detector:latest
docker cp liveness_detector_pip_package_container:/livenessDetector/wrappers/python/dist wrappers/python/dist/
docker rm -f liveness_detector_pip_package_container