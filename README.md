<head>
    <meta charset=utf-8>
    <link href="data/github.css" rel="stylesheet">
</head>

Splash
======

Table of Contents
-----------------

[Introduction](#introduction)

[Installation](#installation)

[Software architecture](#architecture)

[Configuration](#configuration)

[Objects description](#objects)

[User interface](#gui)

[Geometrical calibration](#geometry_calibration)

[Color calibration](#color_calibration)

[Blending](#blending)


<a name="introduction"></a>
Introduction
------------

### About
Splash is a free (as in GPL) modular mapping software. Provided that the user creates a 3D model with UV mapping of the projection surface, Splash will take care of calibrating the videoprojectors (intrinsic and extrinsic parameters, blending and color), and feed them with the input video sources. Splash can handle multiple inputs, mapped on multiple 3D models, and has been tested with up to eight outputs on two graphic cards. It currently runs on a single computer but support for multiple computers mapping together is planned.

Splash has been primarily targeted toward fulldome mapping, and has been extensively tested in this context. Two fulldomes have been mapped: a small dome (3m wide) with 4 projectors, and a big one (20m wide) with 8 projectors. It has also been tested sucessfully as a more regular video-mapping software to project on buildings. Focus has been made on optimization: as of yet Splash can handle flawlessly a 3072x3072@30Hz live video input, and 4096x4096@60Hz on eight outputs (two graphic cards) with a powerful enough cpu and the [HapQ](http://vdmx.vidvox.net/blog/hap) video codec (on a SSD as this codec needs a very high bandwidth). Due to its architecture, higher resolutions are more likely to run smoothly when a single graphic card is used, although nothing higher than 4096x4096@60Hz has been tested yet (well, we tested 6144x6144@60Hz but the drive throughput was not enough to sustain the video bitrate).

Video flows are currently read through shared memory, using the libshmdata library. This makes it compatible with most of the softwares from the SAT Metalab, including Scenic2 which is the best choice to feed Splash film or realtime videos. 3D models can be either loaded from files, or sent to Splash through the same shared memory library. An addon for Blender is included, which allows sending any mesh to Splash from Blender.

### License
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

### Authors
* Emmanuel Durand ([Github](https://github.com/paperManu))([Website](https://emmanueldurand.net))

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

- [OpenGL](http://opengl.org),
- [GLFW](http://glfw.org) to handle the GL context creation,
- [GLM](http://glm.g-truc.net) to ease matrix manipulation,
- [OpenImageIO](http://www.openimageio.org) to load and manipulate image buffers,
- [libshmdata](http://code.sat.qc.ca/redmine/projects/libshmdata) to read video flows from a shared memory,
- [JsonCpp](http://jsoncpp.sourceforge.net) to load and save the configuration,
- [GSL](http://gnu.org/software/gsl) (GNU Scientific Library) to compute calibration,
- [ZMQ](http://zeromq.org) to communicate between the various process involved in a Splash session,
- [Snappy](https://code.google.com/p/snappy/) to handle Hap codec decompression,
- [GPhoto](http://gphoto.sourceforge.net/) to use a camera for color calibration.

A few more libraries are used as submodules in the git repository:

- [ImGui](https://github.com/ocornut/imgui) to draw the GUI,
- [bandit](https://github.com/joakinkarlsson/bandit) to do some unit testing,
- [Piccante](https://github.com/banterle/piccante) to create HDR images.

### Compilation and installation

#### Linux

Splash has currently only been compiled and tested on Ubuntu (version 13.10 and higher) and Mint 15 and higher. GLFW3, OpenImageIO and ShmData are packaged but not (yet) available in the core of these distributions, thus some additional repositories must be added.

Here are some step by step commands to add these repositories on Ubuntu 13.10:

    sudo apt-add-repository ppa:irie/blender
    sudo apt-add-repository ppa:sat-metalab/metalab
    sudo apt-add-repository ppa:pyglfw/pyglfw
    sudo apt-get update

And you are done with dependencies. If your distribution is not compatible with packages from Ubuntu, I'm afraid you will have to compile any missing library by hand for the time being...

Your can now install Splash using the packaged version:

    sudo apt-get install splash

And you should be ready to go!

If you want to get a more up to date version, you can try compiling and installing the latest version from the develop branch of this repository. Note that these version are more likely to contain bugs alongside new features / optimizations.

    sudo apt-get install build-essential git-core subversion cmake automake libtool libxrandr-dev libxi-dev libboost-dev
    sudo apt-get install libglm-dev libglew-dev libopenimageio-dev libshmdata-0.8-dev libjsoncpp-dev libgsl0-dev libzmq3-dev libsnappy-dev libgphoto2-dev
    sudo apt-get install libglfw3-dev

    git clone git://github.com/paperManu/splash
    cd splash
    git checkout develop
    git submodule update --init
    ./autogen.sh && ./configure
    make && sudo make install

You can now try launching Splash:

    splash --help

#### Mac OSX

OSX installation is still a work in progress and has not been extensively tested (far from it!). Also, our current tests have shown that it is far easier to install on OSX version 10.9 or newer, as they switched from libstdc++ (GCC standard library) to libc++ (Clang standard library) as default which seems to solve tedious linking issues.

So, let's start with the installation of the dependencies. Firstly download and install [MacPorts](https://www.macports.org/install.php), after having installed Xcode Developer Tools and XCode Command Line Developer Tools (from the [Apple Developer website](https://developer.apple.com/downloads)).

You can now install the command line tools we will need to download and compile the sources:

    sudo port install automake autoconf libtool cmake git pkgconfig

Grab and install OpenImageIO, the only library needed by Splash which is not packaged in MacPorts:

    sudo port install tiff openexr libpng boost
    git clone https://github.com/OpenImageIO/oiio
    cd oiio
    git checkout Release-1.3.14
    mkdir build && cd build
    cmake ..
    make && sudo make install
    cd ..

We then install Shmdata, which depends on GStreamer:

    sudo port install gstreamer010 gstreamer010-gst-plugins-bad gstreamer010-gst-plugins-base
    sudo port install gstreamer010-gst-plugins-good gstreamer010-gst-plugins-ugly
    git clone https://github.com/nicobou/shmdata
    cd shmdata
    ./autogen.sh && ./configure
    make && sudo make install
    cd ..

Install all the other dependencies:

    sudo port install jsoncpp snappy
    sudo port install gsl zmq cppzmq
    sudo port install glfw glm glew

And then grab and install Splash:

    git clone https://github.com/paperManu/splash
    cd splash
    git submodule update --init
    ./autogen.sh && ./configure
    make && sudo make install

You should now be able to launch Splash:

    splash --help

Remember that it is a very early port to OSX. Please report any issue you encounter!

<a name="architecture"/></a>
Software architecture
---------------------
As said earlier, Splash was made very modular to handle situations outside the scope of domes. Most of the objects can be created and configured from the configuration file (see next section for the syntax). The most important and useful objects are the following:

- *camera*: a virtual camera, corresponding to a given videoprojector. Its view is rendered in an offscreen buffer.
- *window*: a window is meant to be outputted on a given video output. Usually it is linked to a specific camera.
- *image*: a static image.
- *image_shmdata*: a video flow read from a shared memory.
- *mesh*: a mesh (and its UV mapping) corresponding to the projection surface, described as vertices and uv coordinates.
- *mesh_shmdata*: a mesh (and its UV mapping) read from a shared memory.
- *object*: utility class to specify which image will be mapped on which mesh.

The other, less useful (but who knows) objects, are usually created automatically when needed. It is still possible to create them by hand if you have some very specific needs (or if you are a control freak):

- *geometry*: intermediary class holding vertex, uv and normal coordinates of a projection surface, to send them to the GPU.
- *shader*: a shader, describing how an object will be drawn.
- *texture*: a texture handles the GPU memory in which image frames are copied.

These various objects are connected together through *links*, which you have to specify (except for automatically created objects obviously). They are managed in the *World* object for all objects which are not related to OpenGL, and in the various *Scene* objects for all the other ones (although non-GL objects are somewhat mirrored in *Scenes*). In a classic configuration, there is one *World* and as many *Scenes* as there are graphic cards. *World* and *Scenes* run in their own process and communicate through shared memories (for now).

A description of the parameters accepted by each object should be added to this documentation shortly.


<a name="configuration"/></a>
Configuration
-------------
The configuration is stored in a standard Json file, with the exception that C-style comments are accepted (yay!). It contains at least two sections: one for the *World* configuration, and one for the *Scenes*. Any other object is described in additional sections, one for each *Scene*. As I think that a few examples are far better than any description in this case, here are some typical configuration starting with a very simple one output mapping.

Please not that *surface.obj* would be a 3D mesh representation of the projection surface.

### One output

    {
        // Default encoding for text
        "encoding" : "UTF-8",
    
        "world" : {
            "framerate" : 60
        },
    
        "scenes" : [
            {
                "name" : "local",
                "address" : "localhost",
                "spawn" : 1,
                "display" : 0,
                "swapInterval" : 1
            },
        ],
    
        "local" : {
            "cam" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, 2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05
            },
            "win" : {"type" : "window", "fullscreen" : 1},
    
            "mesh" : {"type" : "mesh", "file" : "surface.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "image" : {"type" : "image", "file" : "color_map.png", "benchmark" : 0},
    
            "links" : [
                ["mesh", "object"],
                ["image", "object"],
                ["object", "cam"],
                ["cam", "win"]
            ]
        }
    }

---------------

A rapid walkthrough in this configuration:

        "world" : {
            "framerate" : 60
        },

This single attribute sets the framerate of the *World*, corresponding to the maximum frequency at which new video frames will be sent to the *Scene*. It is not really useful in this case as there is only a static image in the configuration, but it does not hurt.

---------------

        "scenes" : [
            {
                "name" : "local",
                "address" : "localhost",
                "spawn" : 1,
                "display" : 0,
                "swapInterval" : 1
            },
        ],

Description of the sole *Scene* in this configuration file, with its name, its address (only "localhost" is accepted for now). See the objects precise description for more information on the other attributes.

---------------

        "local" : {
            "cam" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, 2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05
            },
            "win" : {"type" : "window", "fullscreen" : 1},
    
            "mesh" : {"type" : "mesh", "file" : "surface.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "image" : {"type" : "image", "file" : "color_map.png", "benchmark" : 0},
    
            "links" : [
                ["mesh", "object"],
                ["image", "object"],
                ["object", "cam"],
                ["cam", "win"]
            ]
        }

This section describes the objects present in the *Scene* named "local". This *Scene* contains a single *camera*, linked to a *window*. An *object* consisting of a *mesh* on which an *image* is mapped is drawn through the *camera*. Also, a *gui* is set.

At the end of this section is a list of all the links between the objects. Note that any *object* is automatically connected to the *gui* of the master *Scene* (i.e. the first one).


### Four outputs, a single *Scene*

    {
        // Default encoding for text
        "encoding" : "UTF-8",
    
        "world" : {
            "framerate" : 300
        },
    
        "scenes" : [
            {
                "name" : "local",
                "address" : "localhost",
                "spawn" : 1,
                "display" : 0,
                "swapInterval" : 0
            }
        ],
    
        "local" : {
            "cam1" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, 2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0
            },
            "cam2" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0
            },
            "cam3" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0
            },
            "cam4" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, 2.0, -0.5],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0
            },
            "win1" : {"type" : "window", "fullscreen" : 0},
            "win2" : {"type" : "window", "fullscreen" : 1},
            "win3" : {"type" : "window", "fullscreen" : 2},
            "win4" : {"type" : "window", "fullscreen" : 3},
    
            "mesh" : {"type" : "mesh", "file" : "sphere.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "shmimage" : {"type" : "image_shmdata", "file" : "/tmp/switcher_default_video_video-0"},
    
            "links" : [
                ["mesh", "object"],
                ["object", "cam1"],
                ["object", "cam2"],
                ["object", "cam3"],
                ["object", "cam4"],
                ["shmimage", "object"],
                ["cam1", "win1"],
                ["cam2", "win2"],
                ["cam3", "win3"],
                ["cam4", "win4"]
            ]
        }
    }

---------------

Apart from the number of *cameras* and *windows*, this configuration is very similar to the previous one with the exception of the use of a *image_shmdata* video input, in place of the *image*.

### Four outputs, two *Scenes*

    {
        // Default encoding for text
        "encoding" : "UTF-8",
    
        "world" : {
            "framerate" : 300
        },
    
        "scenes" : [
            {
                "name" : "first",
                "address" : "localhost",
                "spawn" : 1,
                "display" : 0,
                "swapInterval" : 0
            },
            {
                "name" : "second",
                "address" : "localhost",
                "spawn" : 1,
                "display" : 1,
                "swapInterval" : 0
            }
        ],
    
        "first" : {
            "cam1" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, 2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05
            },
            "cam2" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05
            },
            "win1" : {"type" : "window", "fullscreen" : 0},
            "win2" : {"type" : "window", "fullscreen" : 1},
    
            "mesh" : {"type" : "mesh", "file" : "sphere.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "shmimage" : {"type" : "image_shmdata", "file" : "/tmp/switcher_default_video_video-0"},
    
            "links" : [
                ["mesh", "object"],
                ["object", "cam1"],
                ["object", "cam2"],
                ["shmimage", "object"],
                ["cam1", "win1"],
                ["cam2", "win2"]
            ]
        },
    
        "second" : {
            "cam3" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05
            },
            "cam4" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, 2.0, -0.5],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05
            },
            "win3" : {"type" : "window", "fullscreen" : 2},
            "win4" : {"type" : "window", "fullscreen" : 3},
    
            "mesh" : {"type" : "mesh", "file" : "sphere.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "shmimage" : {"type" : "image_shmdata", "file" : "/tmp/switcher_default_video_video-0"},
    
            "links" : [
                ["mesh", "object"],
                ["object", "cam3"],
                ["object", "cam4"],
                ["shmimage", "object"],
                ["cam3", "win3"],
                ["cam4", "win4"]
            ]
        }
    }

---------------

A few notes on this one:

- the attribute "display" is different for both scenes: the first scene will be drawn by GPU 0, the second scene will be by GPU 1 
- currently, you need to set the same parameters for objects shared between scenes (like "mesh", "object" and "shmimage"). If not, the parameters from the last object will be used. You can also set no parameter at all in all scenes but one, except for the type which has to be set everywhere.


<a name="objects"/></a>
Objects description
-------------------

### camera
Links from:

- object

Links to:

- window

Attributes:

- activateColorLUT [int]: if set to true, the color look up table will used if present
- blendWidth [float]: width of the blending zone, along the camera borders
- blackLevel [float]: minimum value outputted by the camera
- brightness [float]: modify the global brightness of the rendering
- colorTemperature [float]: white point for this camera, in Kelvin (default to 6500.0)
- eye [float, float, float]: position of the camera in the 3D space
- fov [float]: field of view of the camera, in degrees
- principalPoint [float, float]: optical center of the camera, in image coordinates
- size [int, int]: rendering size of the offscreen buffer. Will be updated when connected to a window
- target [float, float, float]: vector indicating the direction of the camera view
- up [float, float, float]: vector of the up direction (used to tilt the view)

The following attributes should not be edited manually:

- calibrationPoints [float array]: list of the calibration points
- colorLUT [float array]: look up table created by the color calibration
- colorMixMatrix [float array]: color mixing matrix created by the color calibration

### geometry
Links from:

- mesh

Links to:

- object

Attributes: None

### image
Links from: None

Links to:

- texture
- window

Attributes:

- benchmark [int]: if set to anything but 0, the *World* will send the image at every iteration even if the image has not been updated
- file [string]: path to the image file to open
- flip [int]: if set to 1, the image will be mirrored vertically
- flop [int]: if set to 1, the image will be mirrored horizontally
- srgb [int]: if set to anything but 0, the image will be considered to be represented in the sRGB color space

### image_shmdata
As this class derives from the class image, it shares all its attributes and behaviors.

Links from: None

Links to:

- texture
- window

Attributes:

- file [string]: path to the shared memory to read from

### mesh
Links from: None

Links to:

- geometry
- object

Attributes:

- benchmark [int]: if set to anything but 0, the *World* will send the mesh at every iteration even if the mesh has not been updated
- file [string]: path to the mesh file to read from

### mesh_shmdata
As this class derives from the class mesh, it shares all its attributes and behaviors

Links from: None

Links to:

- geometry
- object

Attributes:

- file [string]: path to the shared memory to read from

### object
Links from:

- texture
- image
- mesh
- geometry

Links to:

- gui

Attributes:

- color [float, float, float, float]: set the color of the object if "fill" is set to "color"
- fill [string]: if set to:
    - "texture", the object is textured with the linked texture / image
    - "color", the object is colored with the one given by the color attribute
    - "uv", the object is colored according to its texture coordinates
    - "wireframe", the object is displayed as wireframe
    - "window", the object is considered as a window object, with multiple texture input. There is no known reason to use this.
- position [float, float, float]: position of the object in 3D space
- scale [float, float, float]: scale of the object along all three axis
- sideness [int]: if set to:
    - 0, the object is double sided
    - 1, the object is single sided
    - 2, the object is single sided with sides inverted

### scene
A *Scene* is contained in a process and linked to a given GPU. It handles the objects configured in its context, as well as the render loop.

Links from: None

Links to: None

Attributes:

- address [string]: ip address of the scene. Currently only "localhost" is accepted
- display [int]: index of the display for the scene. Usually corresponds to the GPU number.
- spawn [int]: if set to one, the main splash process will spawn the process containing this scene. Otherwise it has to be launched by hand with splash-scene [name]
- name [string]: name of the scene
- swapInterval [int]: if set to:
    - 0: disables any vSync
    - any positive integer: will wait for as many frames between each window update.
    - any negative integer: tries to use vSync except if render is too slow.

### texture
A texture handles an image buffer, be it static or dynamic.

Links from:

- image

Links to:

- object
- window

Attributes:

- resizable [int]: if set to anything but 0, the texture will be resizable

### window
A window is an output to a screen.

Links from:

- camera
- gui
- image
- texture

Links to: None

Attributes:

- decorated [int]: if set to anything but 0, the window will be decorated.
- fullscreen [int]: if set to:
    - -1: disables fullscreen for this window
    - any positive or null integer: tries to set the window to fullscreen on the given screen number
- gamma [float]: exponent to use for the gamma curve.
- layout [int, int, int, int]: specifies how the input textures are ordered. This field is only needed if multiple cameras are connected to a single window, e.g. in the case of a window spanning through multiple projectors.
- position [int, int]: window position in the current display
- size [int, int]: window size
- srgb [int]: if set to anything but 0, sRGB support will be enabled for the window.
- swapInterval [int]: By default, the window swap interval is the same as the scene. If set to:
    - 0: disables any vSync
    - any positive integer: will wait for as many frames between each window update.

### world
The *World* is a single object created at launch by Splash. It handles all the inputs and configuration read / write, and dispatches messages and various buffers to all *Scenes*. It is not possible to add another *World* to a Splash session, although it is possible to specify a few attributes in the configuration file.

Links from: None

Links to: None

Attributes:

- computeBlending [int]: if set to anything but 0, blending will be computed at launch.
- framerate [int]: refresh rate of the *World* main loop. Setting a value higher than the display refresh rate may help reduce latency between video input and output.


<a name="gui"/></a>
User interface
--------------

The user interface is currently separated in two parts: the first one is the json configuration file, where the Cameras, Windows and various Images and Meshes are set. There is currently no way to add any of these objects during runtime, although it is planned.

The second part is a GUI which appears in the very first Window created (this means that if you prefer having the GUI in its own window, you can specify the first window as empty, i.e. no link to anything, this can be very handy). Once Splash is launched, the GUI appears by pressing the tabulation key. The Splash Control Panel is divided in subpanels, as follows:

- Base commands: contains very generic commands, like saving and computing the blending map,
- Shortcuts: shows a list of the shortcuts,
- Timings: shows a few timings regarding the performance of Splash, like the framerate,
- Logs: shows the last log messages, replicated from the ones sent to the console,
- Controls: gives control over the parameters of the various objects,
- Views: shows a global view of the projection surface, which can be switched to any of the configured Cameras. This panel is also used to set geometric calibration up,
- Performance Graph: a more detailed view of the performance of Splash, displaying all the timers implemented in the loop to detect bottlenecks.


<a name="geometry_calibration"/></a>
Geometrical calibration
-----------------------

This is a (very) preliminary guide to projector calibration in Splash, feel free to report any error or useful addition.

The way calibration works in Splash is to reproduce as closely as possible the parameters of the physical projectors onto a virtual camera. This includes its intrinsic parameters (field of view, shifts) as well as its extrinsic parameters (position and orientation). Roughly, to find these parameters we will set a few point - pixel pairs (at least six of them) and then ask Splash to find values which minimizes the squared sum of the differences between point projections and the associated pixel position.

Once the configuration file is set and each videoprojector has an output, do the following on the screen which has the GUI :

- Press 'tab' to make the GUI appear,
- open the camera views, in the Views panel of the GUI,
- while hovering the View panelpress space to navigate through all the cameras, until you find the one you want to calibrage,
- press 'W' to switch the view to wireframe,
- left click on a vertex to select it,
- shift + left click to specify the position where this vertex should be projected. You can move this projection with the arrow keys,
- to delete an erroneous point, ctrl + left click on it,
- continue until you have seven pairs of point - projection. You can orientate the view with the mouse (right click + drag) and zoom in / out with the wheel,
- press 'C' to ask for calibration.

At this point, you should have a first calibration. Pressing 'C' multiple times can help getting a better one, as is adding more pairs of point - projection. You can go back to previous calibration by pressing 'R'.

Once you are happy with the result, you can go back to textured rendering by pressing 'T' and save the configuration by pressing 'Ctrl' + 'S'. It is advised to save after each calibrated camera (you never know...). Also, be aware that the calibration points are saved, so you will be able to update them after reloading the project.


<a name="color_calibration"/></a>
Color calibration
-----------------

Color calibration is done by capturing (automatically) a bunch of photographs of the projected surface, so as to compute a common color and luminance space for all the videoprojectors. Note that Splash must have been compiled with GPhoto support for color calibration to be available. Also, color calibration does not need any geometric calibration to be done yet, although it would not make much sense to have color calibration without geometric calibration.

- Connect a PTP-compatible camera to the computer. The list of compatible cameras can be found [there](http://gphoto.org/proj/libgphoto2/support.php).
- Set the camera in manual mode, chose sensitivity (the lower the better regarding noise) and the aperture (between 1/5.6 and 1/8 to reduce vignetting).
- Open the GUI by pressing 'tab'.
- Go to the Control panel, find the "colorCalibrator" object in the list.
- Set the various options, default values are a good start:
    - colorSamples is the number of samples taken for each channel of each projector,
    - detectionThresholdFactor has an effect on the detection of the position of each projector,
    - equalizeMethod gives the choice between various color balance equalization methods:
        - 0: select a mean color balance of all projectors base balance,
        - 1: select the color balance of the weakest projector,
        - 2: select the color balance which would give the highest global luminance,
    - imagePerHDR sets the number of shots to create the HDR images on which color values will be measured,
    - hdrStep sets the stops between two shots to create an HDR image.
- Press 'O' or click on "Calibrate camera response" in the Base commands panel, to calibrate the camera color response,
- Press 'P' or click on "Calibrate displays / projectors" to launch the projector calibration.

Once done, calibration is automatically activated. It can be turned off by pressing 'L' or by clicking on "Activate correction". If the process went well, the luminance and color balance of the projectors should match more closely. If not, there are a few things to play with:

- Projector detection could have gone wrong. Check in the logs (in the console) that the detected positions make sense. If not, increase or decrease the detectionThresholdFactor and retry.
- The dynamic range of the projectors could be too wide. If so you would notice in the logs that there seem to be a maximum clamping value in the HDR measurements. If so, increase the imagePerHDR value.

While playing with the values, do not hesitate to lower the colorSamples to reduce the calibration time. Once everything seems to run, increase it again to do the final calibration.

Also, do not forget to save the calibration once you are happy with the results!


<a name="blending"/></a>
Blending
--------

This is an easy part, as long as calibration is done: with the GUI open, press 'B' and wait for it to be computed!

Although it is automatic, it is known to have some issues in specific cases which were not solved yet, but seem to be mostly related to the mesh. In particular, try replacing the mesh with a more / less refined one to check if it solves the issue. Note that the blending map is not saved but computed each time a file is loaded (if set so in the configuration file).
