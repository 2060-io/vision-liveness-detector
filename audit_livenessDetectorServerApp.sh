#!/bin/bash

# 1. List your required libraries
libs=(
  libjpeg.so.62
  libpng16.so.16
  libz.so.1
  libtiff.so.5
  libwebp.so.7
  libopenjp2.so.7
  libIlmImf-2_2.so.22
  libImath-2_2.so.12
  libHalf.so.12
  libIex-2_2.so.12
  libIexMath-2_2.so.12
  libIlmThread-2_2.so.12
  libjbig.so.2.1
)

mkdir -p ./lib

# 2. Copy libraries
for lib in "${libs[@]}"; do
  path=$(ldconfig -p | grep "$lib" | head -n1 | awk '{print $NF}')
  if [[ -n "$path" ]]; then
    cp "$path" ./lib/
  else
    echo "WARNING: Could not find $lib"
  fi
done

# 3. Patch RPATH of binary
patchelf --set-rpath '$ORIGIN/lib' ./livenessDetectorServerMod

# 4. Patch RPATH of libs (optional, but recommended for transitive deps)
for f in ./lib/*.so*; do
  patchelf --set-rpath '$ORIGIN' "$f"
done

echo "Done! You can now distribute myapp/ directory."