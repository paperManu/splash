#!/bin/bash

SOURCE_DIR="$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)"

cd ${SOURCE_DIR}/sh4lt
rm -rf build
mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=${SOURCE_DIR}/third_parties -DCMAKE_BUILD_TYPE=Release -DSH4LT_WITH_PYTHON=off ..
ninja && ninja install
