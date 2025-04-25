Splash, a multi-projector video-mapping software
================================================

[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
[![pipeline status](https://gitlab.com/splashmapper/splash/badges/develop/pipeline.svg)](https://gitlab.com/splashmapper/splash/commits/develop)
[![coverage report](https://gitlab.com/splashmapper/splash/badges/develop/coverage.svg)](https://gitlab.com/splashmapper/splash/commits/develop)

For a more complete documentation, go visit the [official website](https://splashmapper.xyz).

## Table of Contents

[Introduction](#introduction)

[Installation](#installation)

[Code contribution](#code-contribution)

[Going forward](#going-forward)


## Introduction

### About
Splash is a free (as in GPL) modular mapping software. Provided that the user creates a 3D model with UV mapping of the projection surface, Splash will take care of calibrating the videoprojectors (intrinsic and extrinsic parameters, blending and color), and feed them with the input video sources. Splash can handle multiple inputs, mapped on multiple 3D models, and has been tested with up to eight outputs on two graphic cards. It currently runs on a single computer but support for multiple computers is planned. It also runs on some ARM hardware, in particular NVIDIA Jetsons are known to work well with Splash.

Although Splash was primarily targeted toward fulldome mapping and has been extensively tested in this context, it can be used for virtually any surface provided that a 3D model of the geometry is available. Multiple fulldomes have been mapped, either by the authors of this software (two small dome (3m wide) with 4 projectors, a big one (20m wide) with 8 projectors) or by other teams. It has also been tested sucessfully as a more regular video-mapping software to project on buildings, or [onto moving objects](https://vimeo.com/268028595).

Splash can read videos from various sources amoung which video files (most common format and Hap variations), video input (such as video cameras, capture cards), NDI video feeds, and shmdata (a shared memory library used to make softwares from the SAT Metalab communicate between each others). An addon for Blender is included which allows for exporting draft configurations and update in real-time the meshes.

### Licenses
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program uses external libraries, some of them being bundled in the source code repository (directly or as submodules). They are located in `external`, and are not necessarily licensed under GPLv3. Please refer to the respective licenses for details.

### Authors
See [AUTHORS.md](docs/Authors.md)

### Project URL
This project can be found either on [its official website](https://splashmapper.xyz), on the [Gitlab repository](https://gitlab.com/splashmapper/splash) or on [Github](https://github.com/paperManu/splash).

### Sponsors
This project is made possible thanks to the [Society for Arts and Technologies](http://www.sat.qc.ca) (also known as SAT) as well as the [Lab148 cooperative](https://lab148.xyz)


## How to use Splash

### Dependencies
Splash relies on a few libraries to get the job done. The mandatory libraries are:

- External dependencies:
  - [FFmpeg](http://ffmpeg.org/) to read and write video files
  - [OpenGL](http://opengl.org), which should be installed by the graphic driver
  - [GSL](http://gnu.org/software/gsl) (GNU Scientific Library) to compute calibration
- External dependencies bundled as submodules:
  - [GLFW](http://glfw.org) to handle the GL context creation
  - [GLM](http://glm.g-truc.net) to ease matrix manipulation
  - [Snappy](https://code.google.com/p/snappy/) to handle Hap codec decompression
  - [ZMQ](http://zeromq.org) to communicate between the various process involved in a Splash session
  - [cppzmq](https://github.com/zeromq/cppzmq.git) for its C++ bindings of ZMQ
  - [JsonCpp](http://jsoncpp.sourceforge.net) to load and save the configuration
  - [stduuid](https://github.com/mariusbancila/stduuid) to help with UUIDs
- Dependencies built at compile-time from submodules:
  - [doctest](https://github.com/onqtam/doctest/) to do some unit testing
  - [ImGui](https://github.com/ocornut/imgui) to draw the GUI
  - [stb_image](https://github.com/nothings/stb) to read images

Some other libraries are optional:

- External dependencies:
  - [libshmdata](http://gitlab.com/sat-mtl/tools/shmdata) to read video flows from a shared memory
  - [portaudio](http://portaudio.com/) to read and output audio
  - [Python](https://python.org) for scripting capabilities
  - [GPhoto](http://gphoto.sourceforge.net/) to use a camera for color calibration
- Dependencies built at compile-time from submodules:
  - [libltc](http://x42.github.io/libltc/) to read timecodes from an audio input

Also, the [Roboto](https://www.fontsquirrel.com/fonts/roboto) font and the [DSEG font family](https://github.com/keshikan/DSEG) are used and distributed under their respective open source licenses.

By default Splash is built and linked against the libraries included as submodules, but it is possible to force it to use the libraries installed on the system. This is described in the next section.

### Installation

Splash can be installed from a pre-built package, or compiled by hand. Newcomers are advised to try the packaged version first, and try to compile it if necessary only.

#### Install from packages

To install from the binary packages, please refer to [Splash documentation](https://splashmapper.xyz).

#### Build from sources

You can also compile Splash by hand, especially if you are curious about its internals or want to tinker with the code (or even, who knows, contribute!). Note that although what follows compiles the develop branch, it is more likely to contain bugs alongside new features / optimizations so if you experience crash you can try with the master branch.

##### Installing the dependencies

The packages necessary to compile Splash are the following:

- Ubuntu 20.04 and newer:

```bash
sudo apt install build-essential git-core cmake cmake-extras libxrandr-dev \
    libxi-dev mesa-common-dev libgsl0-dev libatlas3-base libgphoto2-dev \
    libz-dev libxinerama-dev libxcursor-dev python3-dev yasm portaudio19-dev \
    python3-numpy libopencv-dev libjsoncpp-dev libavcodec-dev libavformat-dev \
    libavutil-dev libswscale-dev ninja-build libwayland-dev libxkbcommon-dev

# Non mandatory libraries needed to link against system libraries only
sudo apt install libglm-dev libsnappy-dev libzmq3-dev
```

- Fedora 39:

If not already installed, add the RPM Fusion additional package repository (needed for some of the following dependencies). This only adds the free repository:

```bash
sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
```

Then install the dependencies:

```bash
sudo dnf install gcc g++ cmake gsl-devel atlas-devel libgphoto2-devel python3-devel \
    yasm portaudio-devel python3-numpy opencv-devel jsoncpp-devel libuuid-devel \
    libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
    mesa-libGL-devel libavcodec-free-devel libavformat-free-devel libavutil-free-devel \
    libswscale-free-devel ninja-build wayland-devel libxkbcommon-devel
```

- Archlinux (not well maintained, please signal any issue):

```bash
pacman -Sy git cmake ninja gcc yasm pkgconfig libxi libxinerama libxrandr libxcursor jsoncpp \
    mesa glm gsl libgphoto2 python3 portaudio zip zlib ffmpeg opencv qt6-base vtk hdf5 glew \
    libxkbcommon fmt
```

- Windows:

On Windows, you need to install a development environment to be able to run Splash. Fortunately there are some very nice ones, and for Splash we use [MSYS2](https://www.msys2.org/). Install it as explained on their website, then run `MSYS2 UCRT64` from the Start menu. This will give you a terminal with the correct environment to build Splash.

To finalize with the dependencies, you need to install a few ones:

```bash
pacman -Sy --needed zip git
pacman -Sy --needed mingw-w64-ucrt-x86_64-{cmake,make,gcc,yasm,pkg-config,jsoncpp,glm,gsl,python3,portaudio,zlib,ffmpeg,zeromq,cppzmq,snappy,opencv,gphoto2}
```

##### Building Splash

Once everything is installed, you can go on with building Splash. To build and link it against the bundled libraries (note that this will not work on Windows):

```bash
git clone --recurse-submodules https://gitlab.com/splashmapper/splash
cd splash
./make_deps.sh
mkdir -p build && cd build
# The BUILD_GENERIC_ARCH flag allows for building an executable which can run on any
# sufficiently modern (less than 15 years) CPU. It is usually safe to remove it but
# people had issues in the past with some arch-specific flags
cmake -GNinja -DBUILD_GENERIC_ARCH=ON ..
ninja
```

Otherwise, to build Splash and link it against the system libraries (this is the path to take on Windows):

```bash
git clone --recurse-submodules https://gitlab.com/splashmapper/splash
cd splash
mkdir -p build && cd build
# The BUILD_GENERIC_ARCH flag allows for building an executable which can run on any
# sufficiently modern (less than 15 years) CPU. It is usually safe to remove it but
# people had issues in the past with some arch-specific flags
cmake -DUSE_SYSTEM_LIBS=ON -DBUILD_GENERIC_ARCH=ON ..
ninja
```

You can now try launching Splash:

```bash
./src/splash --help
```

##### Installing and/or packaging

- Linux: installing from the sources

Once Splash is compiled (see previous subsection), you can install it from the build directory:

```bash
sudo ninja install
# And then it can be run from anywhere 
splash --help
```

- Windows: generating a package ready to be installed and distributed

On Windows, you can install it like on Linux using the `install` build target. But to do things more like they are done on Windows, it is suggested to generate an installation package and then install Splash like any other software. This way it will be available from the Start menu, among other advantages.

First, you need to install the Nullsoft Scriptable Install System (or NSIS), after downloading it from [their webpage](https://nsis.sourceforge.io/Main_Page). This is used by CPack to generate the package. Once installed, run from the build directory:

```bash
ninja package
```

An installation file named `splash-$VERSION-win64.exe` will be generated. Double-click on it from the explorer to run it and install Splash. Once done it can be found in the Start menu, or in `C:\Program Files\splash\bin`.

#### Uninstall Splash (when built from sources)

To uninstall Splash when built from sources, you need to do from the very same directory where Splash has been built:

```bash
cd ${PATH_TO_SPLASH}/build
sudo ninja uninstall
```

##### Advanced configuration

###### Realtime scheduling

If you want to have access to realtime scheduling within Splash, you need to create a group "realtime", add yourself to it and set some limits:

```bash
sudo addgroup realtime
sudo adduser $USER realtime
sudo cp ./data/config/realtime.conf /etc/security/limits.d/
```

And if you want the logs to be written to /var/log/splash.log:

```bash
sudo adduser $USER syslog
```

Then log out and log back in.

###### Attributes default values

If you want to specify some defaults values for the objects, you can set the environment variable SPLASH_DEFAULTS with the path to a file defining default values for given types. An example of such a file can be found in [data/config/splashrc](data/config/splashrc)

And that's it, you can move on to the [First steps](https://splashmapper.xyz/en/tutorials/first_steps.html) page.

###### Wayland support

**Support for the Wayland display server is still a work-in-progress**, and follows the progress of the GLFW library which is used to give a cross-platform way to handle graphic contexts. An example of a current limitation is that if any Splash window is hidden, the whole rendering will be stalled on some Wayland compositors.


## Code contribution

Contributions are welcome ! See [CONTRIBUTING.md](Contributing.md) and [CODE_OF_CONDUCT.md](Code_of_conduct.md) for details.


## Going forward

To learn how to configure and use Splash, the best resource is [its official website](https://splashmapper.xyz).
