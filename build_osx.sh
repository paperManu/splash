#!/bin/bash
./autogen.sh && ./configure --prefix=/tmp/
make -j4 && make install
