Installation
============

## Dependencies
Splash relies on a few libraries to get the job done. The mandatory libraries are:

- External dependencies:
  - [OpenGL](http://opengl.org), which should be installed by the graphic driver,
  - [GSL](http://gnu.org/software/gsl) (GNU Scientific Library) to compute calibration,
- External dependencies bundled as submodules:
  - [FFmpeg](http://ffmpeg.org/) to read video files,
  - [GLFW](http://glfw.org) to handle the GL context creation,
  - [GLM](http://glm.g-truc.net) to ease matrix manipulation,
  - [Snappy](https://code.google.com/p/snappy/) to handle Hap codec decompression,
  - [ZMQ](http://zeromq.org) to communicate between the various process involved in a Splash session,
  - [cppzmq](https://github.com/zeromq/cppzmq.git) for its C++ bindings of ZMQ
- Dependencies built at compile-time from submodules:
  - [doctest](https://github.com/onqtam/doctest/) to do some unit testing,
  - [ImGui](https://github.com/ocornut/imgui) to draw the GUI,
  - [JsonCpp](http://jsoncpp.sourceforge.net) to load and save the configuration,
  - [stb_image](https://github.com/nothings/stb) to read images.

Some other libraries are optional:

- External dependencies:
  - [libshmdata](http://gitlab.com/sat-metalab/shmdata) to read video flows from a shared memory,
  - [portaudio](http://portaudio.com/) to read and output audio,
  - [Python](https://python.org) for scripting capabilities,
  - [GPhoto](http://gphoto.sourceforge.net/) to use a camera for color calibration.
- Dependencies built at compile-time from submodules:
  - [libltc](http://x42.github.io/libltc/) to read timecodes from an audio input,

Also, the [Roboto](https://www.fontsquirrel.com/fonts/roboto) font is used and distributed under the Apache license.

By default Splash is built and linked against the libraries included as submodules, but it is possible to force it to use the libraries installed on the system. This is described in the next section.


## Compilation and installation

This section describes how to install Splash. To get information about how to install the Blender addons, refer to the dedicated section in the [Users interface page](../User_Interface/#blender-addons)

### Packages

The easiest way to install and test Splash is by using the [Flatpak](https://flatpak.org) archive, which is compatible with most Linux distributions. The latest versio ncan be downloaded from [Flathub](https://flathub.org/apps/details/com.gitlab.sat_metalab.Splash), and installed as follows on Ubuntu:

```bash
sudo apt install flatpak
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install com.gitlab.sat_metalab.Splash
```

Splash should now be available from your application menu (this may require to logout / log back in). A known limitation of the Flatpak package is that it has no access to Jack, and cannot use multiple GPUs.

The current release of Splash is also packaged for Ubuntu (version 20.04) and derived. This is done through a Debian archive, the latest version can be download from the `Download` menu or [here](./splash.deb) and installs with :

```bash
sudo apt install <download path>/splash.deb
```

### Linux

You can also compile Splash by hand, especially if you are curious about its internals or want to tinker with the code (or even, who knows, contribute!). Note that although what follows compiles the develop branch, it is more likely to contain bugs alongside new features / optimizations so if you experience crash you can try with the master branch.

The packages necessary to compile Splash are the following:
- Ubuntu 20.04 and derivatives:

```bash
sudo apt install build-essential git-core cmake libxrandr-dev libxi-dev \
    mesa-common-dev libgsl0-dev libatlas3-base libgphoto2-dev libz-dev \
    libxinerama-dev libxcursor-dev python3-dev yasm portaudio19-dev \
    python3-numpy libopencv-dev libjsoncpp-dev libx264-dev \
    libx265-dev

# Non mandatory libraries needed to link against system libraries only
sudo apt install libglfw3-dev libglm-dev libavcodec-dev libavformat-dev \
    libavutil-dev libswscale-dev libsnappy-dev libzmq3-dev libzmqpp-dev
```

- Ubuntu 18.04 and derivatives:
Splash needs a recent version of CMake (3.12 or newer), as well as GCC 8 or newer. For CMake this implies using [Kitware's PPA](https://apt.kitware.com/):

```bash
sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt install cmake gcc-8 g++-8
```

Then install the dependencies listed for Ubuntu 20.04. Later, when building Splash, you will need to specify which compiler to use to CMake:

```bash
(...)
CC=gcc-8 CXX=g++-8 cmake ..
(...)
```

- Archlinux (not well maintained, please signal any issue):

```bash
pacman -Sy git cmake make gcc yasm pkgconfig libxi libxinerama libxrandr libxcursor libjsoncpp
pacman -Sy mesa glm gsl libgphoto2 python3 portaudio zip zlib
```

Once everything is installed, you can go on with building Splash. To build and link it against the bundled libraries:

```bash
git clone https://gitlab.com/sat-metalab/splash
cd splash
git submodule update --init
./make_deps.sh
mkdir -p build && cd build
cmake ..
make -j$(nproc) && sudo make install
```

Otherwise, to build Splash and link it against the system libraries:

```bash
git clone https://gitlab.com/sat-metalab/splash
cd splash
git submodule update --init
mkdir -p build && cd build
cmake -DUSE_SYSTEM_LIBS=ON ..
make -j$(nproc) && sudo make install
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

If you want to specify some defaults values for the objects, you can set the environment variable SPLASH_DEFAULTS with the path to a file defining default values for given types. An example of such a file can be found in [data/config/splashrc](https://gitlab.com/sat-metalab/splash/tree/master/data/config/splashrc)

And that's it, you can move on the the [Walkthrough](../Walkthrough) page.
