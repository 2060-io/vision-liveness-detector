#!/bin/bash

set -e

SRCDIR="../src/livenessDetector"
EXE="gesture_test"

g++ -std=c++17 -Wall -Werror \
    $SRCDIR/gesture.cc $SRCDIR/gesture_test.cc \
    -o $EXE

echo "Compilation succeeded. Running test:"
echo "----------------------------------------"
./$EXE