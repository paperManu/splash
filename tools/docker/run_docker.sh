#!/bin/bash

git_branch=${1:-master}

if [ ! -f "build_package.sh" ]; then
    echo "This script should be ran from the splash/tools/docker directory"
    exit 1
fi

if [ ! $(which docker) ]; then
    echo "docker.io has to be installed for this script to run"
    exit 1
fi

docker build -t metalab/splash:build -f Dockerfile .
docker run --rm -t -v ${PWD}:/pkg:rw metalab/splash:build $git_branch
