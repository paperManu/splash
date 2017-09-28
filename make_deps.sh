#!/bin/bash

if [ ! -f README.md -o $(cat README.md | grep -c splash) == 0 ]; then
    echo "This script must be executed from the Splash source directory."
    exit 1
fi

# Make sure the submodules have been initialized and updated
git submodule update --init
cd external

# We use the same prefix for all built libs
if [ ! -d third_parties ]; then
    mkdir third_parties
fi

# Snappy
echo "Building Snappy..."
./build_snappy.sh

# FFmpeg
echo "Building FFmpeg..."
./build_ffmpeg.sh

# ZMQ
echo "Building ZMQ..."
./build_zmq.sh
