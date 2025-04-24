#!/bin/bash

# Ensure the script stops if any command fails
set -e

cd build/mediapipe
bazel build --define MEDIAPIPE_DISABLE_GPU=1 //livenessDetectorServerApp:livenessDetectorServer

echo "Successfully built livenessDetectorServer."

# TODO: Make this script compatible with platforms other than linux x86 64
mkdir -p ../../wrappers/python/liveness_detector/server
cp bazel-bin/livenessDetectorServerApp/livenessDetectorServer ../../wrappers/python/liveness_detector/server
cp -R livenessDetectorServerApp/gestures ../../wrappers/python/liveness_detector
cp -R livenessDetector/*.json ../../wrappers/python/liveness_detector

echo "Successfully copied files to python wrapper directory."

cd ../../wrappers/python

python setup.py bdist_wheel --python-tag=py3 --plat-name=manylinux2014_x86_64

echo "Successfully built python package."

