# Use the ubuntu:22.04 base image
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Update package lists and install necessary packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        gcc g++ \
        ca-certificates \
        curl \
        ffmpeg \
        git \
        wget \
        unzip \
        nodejs \
        npm \
        python3-dev \
        python3-opencv \
        python3-pip \
        libopencv-dev \
        software-properties-common \
        openjdk-8-jdk \
        mesa-common-dev \
        libegl1-mesa-dev \
        libgles2-mesa-dev \
        mesa-utils && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Clang 16
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 16 && \
    ln -sf /usr/bin/clang-16 /usr/bin/clang && \
    ln -sf /usr/bin/clang++-16 /usr/bin/clang++ && \
    ln -sf /usr/bin/clang-format-16 /usr/bin/clang-format

# Upgrade pip and install Python dependencies
RUN pip3 install --upgrade setuptools
RUN pip3 install wheel
RUN pip3 install future
RUN pip3 install absl-py "numpy<2" jax[cpu] opencv-contrib-python protobuf==3.20.1
RUN pip3 install six==1.14.0
RUN pip3 install tensorflow
RUN pip3 install tf_slim

# Create a symbolic link for python
RUN ln -s /usr/bin/python3 /usr/bin/python

# Set up Bazel
ARG BAZEL_VERSION=6.5.0
RUN mkdir /bazel && \
    wget --no-check-certificate -O /bazel/installer.sh "https://github.com/bazelbuild/bazel/releases/download/${BAZEL_VERSION}/bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh" && \
    wget --no-check-certificate -O /bazel/LICENSE.txt "https://raw.githubusercontent.com/bazelbuild/bazel/master/LICENSE" && \
    chmod +x /bazel/installer.sh && \
    /bazel/installer.sh && \
    rm -f /bazel/installer.sh

# Set environment variables for Clang
ENV CC=clang
ENV CXX=clang++

# Copy the local repository content into the Docker image
WORKDIR /mediapipe
COPY . /vision-liveness-detector

# Copy necessary parts
RUN cp -r /vision-liveness-detector/livenessDetector .
RUN cp -r /vision-liveness-detector/livenessDetectorServerApp .
RUN git apply /vision-liveness-detector/patch/opencv_from_distro_apt.patch

# Build the project with Bazel
#RUN bazel build --jobs=5 -c opt --linkopt -s --strip always --define MEDIAPIPE_DISABLE_GPU=1 livenessDetectorServerApp:livenessDetectorServer