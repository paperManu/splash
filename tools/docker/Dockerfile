FROM ubuntu:xenial
MAINTAINER Emmanuel Durand <emmanueldurand@protonmail.com>

RUN apt update && apt upgrade -y
RUN apt install --assume-yes \
    zip \
    build-essential \
    git-core \
    subversion \
    cmake \
    automake \
    libtool \
    libxrandr-dev \
    libxi-dev \
    libglm-dev \
    libgsl0-dev \
    libatlas3-base \
    libzmq3-dev \
    libsnappy-dev \
    libgphoto2-dev \
    libgl1-mesa-dev \
    libxinerama-dev \
    libxcursor-dev \
    python3-dev \
    portaudio19-dev \
    libopencv-dev \
    yasm

VOLUME /pkg

COPY build_package.sh /tmp/

ENTRYPOINT ["bash", "-c", "/tmp/build_package.sh ${*}", "--"]
