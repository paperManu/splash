#!/bin/bash

if hash nproc; then
    CPU_COUNT=$(nproc)
elif hash gnproc; then
    CPU_COUNT=$(gnproc)
else
    CPU_COUNT=1
fi

if [ ! -d cppcheck ]; then
    git clone https://github.com/danmar/cppcheck/ cppcheck
fi

cd cppcheck || exit
if [ ! -f cppcheck ]; then
    git checkout 1.81
    make -j${CPU_COUNT} SRCDIR=build CFGDIR=cfg HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function"
fi
