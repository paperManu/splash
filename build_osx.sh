#!/bin/bash
./autogen.sh && ./configure --prefix=/tmp/
make && make install
