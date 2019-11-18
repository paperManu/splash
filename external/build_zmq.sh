#!/bin/bash

EXTERNALS_DIR=$(pwd)

if hash nproc; then
    CPU_COUNT=$(nproc)
elif hash gnproc; then
    CPU_COUNT=$(gnproc)
else
    CPU_COUNT=1
fi

cd zmq
rm -rf build
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=${EXTERNALS_DIR}/third_parties -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib ..
make -j${CPU_COUNT} && make install
