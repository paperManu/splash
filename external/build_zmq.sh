#!/bin/bash

SOURCE_DIR="$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)"

cd ${SOURCE_DIR}/zmq
rm -rf build
mkdir build && cd build
cmake -GNinja -DCMAKE_POLICY_VERSION_MINIMUM=3.10 -DCMAKE_INSTALL_PREFIX=${SOURCE_DIR}/third_parties -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_TESTS=OFF -DWITH_LIBSODIUM=OFF -DENABLE_WS=OFF ..
ninja && ninja install
