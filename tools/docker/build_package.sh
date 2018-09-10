#!/bin/bash

git_branch=${1:-master}

if [ ! -d "/pkg" ]; then
    echo "This script should not be run outside of the Docker container"
    exit 1
fi

if [ ! -d "splash" ]; then
    rm -rf splash
fi

echo "Checking out branch $git_branch"

git clone https://gitlab.com/sat-metalab/splash.git
cd splash
git checkout $git_branch
git submodule update --init
./make_deps.sh

if [ -d "build" ]; then
    rm -rf build
fi
mkdir build && cd build

cmake -DBUILD_GENERIC_ARCH=ON -DCMAKE_INSTALL_PREFIX=/usr ..
make -j$(nproc) && make package

if [ ! -f splash-*.deb ]; then
    echo "The package has not been built, an error seems to have occured"
    exit 1
else
    cp splash-*.deb /pkg
fi

