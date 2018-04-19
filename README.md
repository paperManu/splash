Splash, a multi-projector video-mapping software
================================================

[![Build status](https://gitlab.com/sat-metalab/splash/badges/develop/build.svg)](https://gitlab.com/sat-metalab/splash/commits/develop)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/3544.svg)](https://scan.coverity.com/projects/papermanu-splash)

For a more complete documentation, go visit the [wiki](https://github.com/paperManu/splash/wiki).

Table of Contents
-----------------

[Introduction](#introduction)

[Installation](#installation)

[Going forward](#goingforward)


<a name="introduction"></a>
Introduction
------------

### About
Splash is a free (as in GPL) modular mapping software. Provided that the user creates a 3D model with UV mapping of the projection surface, Splash will take care of calibrating the videoprojectors (intrinsic and extrinsic parameters, blending and color), and feed them with the input video sources. Splash can handle multiple inputs, mapped on multiple 3D models, and has been tested with up to eight outputs on two graphic cards. It currently runs on a single computer but support for multiple computers mapping together is planned.

Splash has been primarily targeted toward fulldome mapping, and has been extensively tested in this context. Two fulldomes have been mapped: a small dome (3m wide) with 4 projectors, and a big one (20m wide) with 8 projectors. It has also been tested sucessfully as a more regular video-mapping software to project on buildings. Focus has been made on optimization: as of yet Splash can handle flawlessly a 3072x3072@30Hz live video input, and 4096x4096@60Hz on eight outputs (two graphic cards) with a powerful enough cpu and the [HapQ](http://vdmx.vidvox.net/blog/hap) video codec (on a SSD as this codec needs a very high bandwidth). Due to its architecture, higher resolutions are more likely to run smoothly when a single graphic card is used, although nothing higher than 4096x4096@60Hz has been tested yet (well, we tested 6144x6144@60Hz but the drive throughput was not enough to sustain the video bitrate).

Splash can read videos from various sources amoung which video files (most common format and Hap variations), video input (such as video cameras and capture cards), Syphon on OSX, and Shmdata (a shared memory library used to make softwares from the SAT Metalab communicate between each others). An addon for Blender is included which allows for exporting draft configurations and update in real-time the meshes. It also handles automatically a few things:
- semi automatic geometric calibration of the video-projectors,
- automatic calibration of the blending between them,
- automatic colorimetric calibration (with a gPhoto compatible camera)

### License
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

### Authors
* Emmanuel Durand ([Github](https://github.com/paperManu))([Website](https://emmanueldurand.net))
* Jérémie Soria ([Github](https://github.com/eldaranne))

### Projet URL
This project can be found either on the [SAT Metalab repository](http://code.sat.qc.ca/redmine/projects/splash) or on [Github](https://github.com/paperManu/splash).

### Sponsors
This project is made possible thanks to the [Society for Arts and Technologies](http://www.sat.qc.ca) (also known as SAT).
Thanks to the Ministère du Développement économique, de l'Innovation et de l'Exportation du Québec (MDEIE).


<a name="installation"/></a>
Installation
------------

### Dependencies
Splash relies on a few libraries to get the job done. These libraries are:

- [OpenGL](http://opengl.org), which should be installed by the graphic driver,
- [libshmdata](http://code.sat.qc.ca/redmine/projects/libshmdata) to read video flows from a shared memory,
- [GSL](http://gnu.org/software/gsl) (GNU Scientific Library) to compute calibration,
- [portaudio](http://portaudio.com/) to read and output audio,
- [Python](https://python.org) for scripting capabilities,
- [GPhoto](http://gphoto.sourceforge.net/) to use a camera for color calibration.

A few more libraries are used as submodules in the git repository:

- [FFmpeg](http://ffmpeg.org/) to read video files,
- [GLFW](http://glfw.org) to handle the GL context creation,
- [GLM](http://glm.g-truc.net) to ease matrix manipulation,
- [ImGui](https://github.com/ocornut/imgui) to draw the GUI,
- [doctest](https://github.com/onqtam/doctest/) to do some unit testing,
- [Piccante](https://github.com/banterle/piccante) to create HDR images,
- [Snappy](https://code.google.com/p/snappy/) to handle Hap codec decompression,
- [libltc](http://x42.github.io/libltc/) to read timecodes from an audio input,
- [JsonCpp](http://jsoncpp.sourceforge.net) to load and save the configuration,
- [stb_image](https://github.com/nothings/stb) to read images.
- [ZMQ](http://zeromq.org) to communicate between the various process involved in a Splash session,
- [cppzmq](https://github.com/zeromq/cppzmq.git) for its C++ bindings of ZMQ

Also, the [Roboto](https://www.fontsquirrel.com/fonts/roboto) font is used and distributed under the Apache license.

### Compilation and installation

#### Linux

The current release of Splash has currently only been compiled and tested on Ubuntu (version 16.04) and Mint 18 and higher. The easy way to install it is to get the Debian archive from the [release page](https://github.com/paperManu/splash/releases), and install it with :

```bash
sudo apt install <download path>/splash-<version>-Linux.deb
```

You can also compile Splash by hand, especially if you are curious about its internals or want to tinker with the code (or even, who knows, contribute!). Note that although what follows compiles the develop branch, it is more likely to contain bugs alongside new features / optimizations so if you experience crash you can try with the master branch.

The packages necessary to compile Splash are the following:
- Ubuntu and derivatives:

```bash
sudo apt install build-essential git-core cmake libxrandr-dev libxi-dev
sudo apt install mesa-common-dev libglm-dev libgsl0-dev libatlas3-base libgphoto2-dev libz-dev
sudo apt install libxinerama-dev libxcursor-dev python3-dev yasm portaudio19-dev
sudo apt install python3-numpy
```

- Archlinux (not well maintained, please signal any issue):

```bash
pacman -Sy git cmake make gcc yasm pkgconfig libxi libxinerama libxrandr libxcursor
pacman -Sy mesa glm gsl libgphoto2 python3 portaudio zip zlib
```

Once everything is installed, you can go on with building Splash:

```bash
git clone git://github.com/paperManu/splash
cd splash
git submodule update --init
./make_deps.sh
mkdir -p build && cd build
cmake ..
make && sudo make install
```

You can now try launching Splash:

```bash
splash --help
```

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

If you want to specify some defaults values for the objects, you can set the environment variable SPLASH_DEFAULTS with the path to a file defining default values for given types. An example of such a file can be found in [data/splashrc](data/splashrc)

And that's it, you can move on the the [Walkthrough](https://github.com/paperManu/splash/wiki/Walkthrough) page.

<a name="goingforward"/></a>
Going forward
-------------

To learn how to configure and use Splash, the best resource currently is the Wiki page on [Github](https://github.com/paperManu/splash/wiki).
