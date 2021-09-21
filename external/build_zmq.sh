#!/bin/bash

SOURCE_DIR="$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)"

if hash nproc; then
    CPU_COUNT=$(nproc)
elif hash gnproc; then
    CPU_COUNT=$(gnproc)
else
    CPU_COUNT=1
fi

cd ${SOURCE_DIR}/zmq
rm -rf build
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=${SOURCE_DIR}/third_parties -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib -DWITH_LIBSODIUM=OFF -DWITH_TLS=OFF -DENABLE_WS=OFF ..
make -j${CPU_COUNT} && make install
