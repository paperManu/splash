#!/bin/bash

EXTERNALS_DIR=$(pwd)

cd ffmpeg
./configure --enable-libsnappy --enable-gpl --disable-doc --disable-ffserver --disable-ffplay --disable-ffprobe --disable-ffmpeg --prefix="$EXTERNALS_DIR/third_parties"
make -j$(nproc)
make install
