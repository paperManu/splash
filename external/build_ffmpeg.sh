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
./configure --enable-gpl --disable-doc --disable-ffserver --disable-ffplay --disable-ffprobe --disable-ffmpeg --prefix="$EXTERNALS_DIR/third_parties"
make clean
make -j${CPU_COUNT}
make install
