FROM ubuntu:artful
MAINTAINER Emmanuel Durand <emmanueldurand@protonmail.com>

RUN apt update && apt upgrade -y
RUN apt install --assume-yes \
    zip \
    build-essential \
    git-core \
    cmake \
    automake \
    libtool \
    libxrandr-dev \
    libxi-dev \
    libgsl0-dev \
    libatlas3-base \
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
