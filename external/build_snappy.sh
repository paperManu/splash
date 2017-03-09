#!/bin/bash

EXTERNALS_DIR=$(pwd)

if hash nproc; then
    CPU_COUNT=$(nproc)
elif hash gnproc; then
    CPU_COUNT=$(gnproc)
else
    CPU_COUNT=1
fi

cd snappy
./autogen.sh
./configure --prefix=${EXTERNALS_DIR}/third_parties --disable-shared
make -j${CPU_COUNT}
make install
