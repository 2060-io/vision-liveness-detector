#!/bin/bash

# Ensure the script stops if any command fails
set -e

# Define the source and destination directories
SOURCE1="livenessDetector"
SOURCE2="livenessDetectorServerApp"
TARGET="mediapipe/"

# Copy the source directories into the target location within MediaPipe
cp -r "$SOURCE1" "$TARGET"
cp -r "$SOURCE2" "$TARGET"

echo "Successfully copied $SOURCE1 and $SOURCE2 into $TARGET."