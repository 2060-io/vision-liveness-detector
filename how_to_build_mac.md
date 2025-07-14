# Full Guide: Building on macOS with Locally Compiled OpenCV

This guide:

* Builds **OpenCV** from source in a **temporary local folder**.
* Sets up **MediaPipe** to use it via `macos_opencv`.
* Avoids system-wide installation.

---

## Step 1: Install Prerequisites

```bash
xcode-select --install
```

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

```bash
brew install python cmake git pkg-config freetype harfbuzz bazelisk
```

```bash
python3 -m venv ~/mediapipe_build/venv
source ~/mediapipe_build/venv/bin/activate
pip install numpy
```

---

## Step 2: Create Project Folders

```bash
mkdir -p ~/mediapipe_build/opencv_source
mkdir -p ~/mediapipe_build/mediapipe_workspace
```

---

## Step 3: Clone and Build OpenCV

```bash
cd ~/mediapipe_build/opencv_source
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
mkdir -p build && cd build
```

### Build with custom modules and freetype

```bash
cmake ../opencv \
    -DOPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules \
    -DWITH_FREETYPE=ON \
    -DOPENCV_ENABLE_NONFREE=ON \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../opencv_local_install
```

```bash
make -j$(sysctl -n hw.logicalcpu)
make install
```

> OpenCV will be installed in:
> `~/mediapipe_build/opencv_source/opencv_local_install`

---

## Step 4: Clone MediaPipe

```bash
cd ~/mediapipe_build/mediapipe_workspace
git clone https://github.com/google/mediapipe.git
cd mediapipe
```

---

## Step 5: Configure `WORKSPACE` to Use Your Local OpenCV

Edit the file: `WORKSPACE`
Modify or add this entry:

```python
new_local_repository(
    name = "macos_opencv",
    build_file = "@//third_party:opencv_macos.BUILD",
    path = "/Users/YOUR_USER/mediapipe_build/opencv_source/opencv_local_install",  # replace with full absolute path
)
```

> You can use the absolute form:  
> `/Users/YOUR_USER/mediapipe_build/opencv_source/opencv_local_install`  
> _(Replace `YOUR_USER` with your username or use command line `echo $HOME` as `$HOME/mediapipe_build/...` when pasting.)_

---

## Step 6: Create or Edit `third_party/opencv_macos.BUILD`

Create the file:
`~/mediapipe_build/mediapipe_workspace/mediapipe/third_party/opencv_macos.BUILD`

```python
PREFIX = "./"

cc_library(
    name = "opencv",
    srcs = glob([
        "lib/libopencv_core.dylib",
        "lib/libopencv_calib3d.dylib",
        "lib/libopencv_features2d.dylib",
        "lib/libopencv_highgui.dylib",
        "lib/libopencv_imgcodecs.dylib",
        "lib/libopencv_imgproc.dylib",
        "lib/libopencv_video.dylib",
        "lib/libopencv_videoio.dylib",
        "lib/libopencv_freetype.dylib",
        "lib/libopencv_face.dylib",          # Optional (nonfree)
        "lib/libopencv_xfeatures2d.dylib",   # Optional (nonfree)
    ]),
    hdrs = glob(["include/opencv4/opencv2/**/*.h*"]),
    includes = ["include/opencv4"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
)
```

> You can adjust this list based on which `.dylib` files were built. Run:
>
> ```bash
> ls ~/mediapipe_build/opencv_source/opencv_local_install/lib
> ```

---

## Step 7: Copy the src/livenessDetector and src/livenessDetectorServerApp folders to mediapipe folder

```bash
cp -r LIVENESS_DETECTOR_PATH/src/livenessDetector ~/mediapipe_build/mediapipe_workspace/mediapipe/
cp -r LIVENESS_DETECTOR_PATH/src/livenessDetectorServerApp ~/mediapipe_build/mediapipe_workspace/mediapipe/
```

---

## Step 8: Build livenessDetectorServerApp

```bash
cd ~/mediapipe_build/mediapipe_workspace/mediapipe
bazel build --define MEDIAPIPE_DISABLE_GPU=1 //livenessDetectorServerApp:livenessDetectorServer
```

---

## Step 9: Generated file

The generated file will be located at:  
`~/mediapipe_build/mediapipe_workspace/mediapipe/bazel-bin/livenessDetectorServerApp/livenessDetectorServer`

## Step 10: Set the Path

```bash
export DYLD_LIBRARY_PATH=/your/opencv_local_install/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}
```

---
