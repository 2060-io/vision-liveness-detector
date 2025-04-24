#!/bin/bash

# Ensure the script stops if any command fails
set -e

# Define the source and destination directories
SOURCE1="src/livenessDetector"
SOURCE2="src/livenessDetectorServerApp"
TARGET_PATH="build/mediapipe/"
MEDIAPIPE_URL="https://github.com/google-ai-edge/mediapipe"
MEDIAPIPE_TAG="v0.10.20"

# Clone Mediapipe, which will be used as a base for compilation
if [ ! -d "$TARGET_PATH" ] ; then
    git clone --depth 1 --branch $MEDIAPIPE_TAG $MEDIAPIPE_URL $TARGET_PATH
fi

# Copy the source directories into the target location within MediaPipe
cp -r "$SOURCE1" "$TARGET_PATH"
cp -r "$SOURCE2" "$TARGET_PATH"

echo "Successfully copied $SOURCE1 and $SOURCE2 into $TARGET_PATH."