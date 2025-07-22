#!/bin/bash

set -e

SRCDIR="../src/livenessDetector"
EXE="gesture_detector_test"

g++ -std=c++17 -Wall -Werror \
    -I$SRCDIR $SRCDIR/gesture.cc $SRCDIR/gesture_detector.cc $SRCDIR/gesture_detector_test.cc \
    -o $EXE

echo "Compilation succeeded. Running test:"
echo "----------------------------------------"
./$EXE
