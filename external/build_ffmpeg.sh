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
./configure --enable-gpl --enable-libx264 --enable-libx265 \
    --disable-avdevice --disable-swresample --disable-postproc --disable-avfilter \
    --disable-doc --disable-ffplay --disable-ffprobe --disable-ffmpeg \
    --prefix="$EXTERNALS_DIR/third_parties"
make clean
make -j${CPU_COUNT}
make install
