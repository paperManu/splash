#!/bin/bash

EXTERNALS_DIR=$(pwd)

if hash nproc; then
    CPU_COUNT=$(nproc)
elif hash gnproc; then
    CPU_COUNT=$(gnproc)
else
    CPU_COUNT=1
fi

cd ffmpeg
git clean -X -f  # FFmpeg has a tendency to not update pkgconfig files, this takes care of that
./configure --disable-avdevice --disable-swresample --disable-postproc --disable-avfilter --enable-gpl --disable-doc --disable-ffserver --disable-ffplay --disable-ffprobe --disable-ffmpeg --prefix="$EXTERNALS_DIR/third_parties"
make clean
make -j$(nproc)
make install
