#!/bin/bash

git_branch=${1:-master}

if [ ! -f "build_package.sh" ]; then
    echo "This script should be ran from the splash/tools/docker directory"
    exit 1
fi

if [ ! $(which docker-compose) ]; then
    echo "docker.io and docker-compose have to be installed for this script to run"
    exit 1
fi

docker-compose build --no-cache --force-rm --pull
docker-compose run package /pkg/build_package.sh $git_branch
