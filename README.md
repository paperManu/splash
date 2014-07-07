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

[Calibration](#calibration)

[Blending](#blending)


<a name="introduction"></a>
Introduction
------------

### About
Splash is a free (as in GPL) modular mapping software. Provided that the user creates a 3D model with UV mapping of the projection surface, Splash will take care of calibrating the videoprojectors (intrinsic and extrinsic parameters, blending), and feed them with the input video sources. Splash can handle multiple inputs, mapped on multiple 3D models, and has been tested with up to eight outputs on two graphic cards. It currently runs on a single computer but support for multiple computers mapping together is planned.

Splash has been primarily targeted toward fulldome mapping, and has been extensively tested in this context. Two fulldomes have been mapped: a small dome (3m wide) with 4 projectors, and a big one (20m wide) with 8 projectors. Also, the focus has been made on optimization. As of yet Splash can handle flawlessly a 3072x3072@30Hz video input, and 4096x4096@60Hz on eight outputs (two graphic cards) with a powerful enough cpu and the [HapQ](http://vdmx.vidvox.net/blog/hap) video codec (on a SSD as this codec needs a very high bandwidth). Due to its architecture, higher resolutions are more likely to run smoothly when a single graphic card is used, although nothing higher than 4096x4096@60Hz has been tested yet.

Video flows are currently read through shared memory, using the libshmdata library. This makes it compatible with most of the softwares from the SAT Metalab, including Scenic2 which is the best choice to feed Splash film or realtime videos.

### License
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

### Authors
* Emmanuel Durand ([Github](https://github.com/paperManu))

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
- [OpenMesh](http://openmesh.org) to load and manipulate meshes,
- [OpenImageIO](http://www.openimageio.org) to load and manipulate image buffers,
- [libshmdata](http://code.sat.qc.ca/redmine/projects/libshmdata) to read video flows from a shared memory,
- [JsonCpp](http://jsoncpp.sourceforge.net) to load and save the configuration,
- [GSL](http://gnu.org/software/gsl) (GNU Scientific Library) to compute calibration,
- [ZMQ](http://zeromq.org) to communicate between the various process involved in a Splash session,
- [Snappy](https://code.google.com/p/snappy/) to handle Hap codec decompression.

A few more libraries are used as submodules in the git repository:

- [GLV](http://mat.ucsb.edu/glv) to draw a (rather simple) GUI,
- [libsimdpp](https://github.com/p12tic/libsimdpp) to use SIMD instructions (currently in YUV to RGB conversion),
- [bandit](https://github.com/joakinkarlsson/bandit) to do some unit testing.

### Dependencies installation
Splash has currently only been compiled and tested on Ubuntu (version 13.10 and higher) and Mint 15 and higher. GLFW3, OpenMesh, OpenImageIO and ShmData are packaged but not (yet) available in the core of these distributions, thus some additional repositories must be added.

Here are some step by step commands to add these repositories on Ubuntu 13.10:

    sudo apt-add-repository ppa:irie/blender
    sudo apt-add-repository ppa:sat-metalab/metalab
    sudo apt-add-repository ppa:keithw/glfw3
    sudo apt-get update

And you are done with dependencies. If your distribution is not compatible with packages from Ubuntu, I'm afraid you will have to compile any missing library by hand for the time being...

### Compilation and installation
Your first option to install Splash is to use the packaged version:

    sudo apt-get install splash

And you should be ready to go!

If you want to get a more up to date version, you can try compiling and installing the latest version from the develop branch of this repository. Note that these version are more likely to contain bugs alongside new features / optimizations. Also, OpenMesh is not in the default Ubuntu repository, so it has been packaged by the Metalab and is only available for Ubuntu 13.10 yet. If you want to install Splash on another revision you have to compile OpenMesh by yourself.

    sudo apt-get install build-essential git-core subversion cmake automake libtool libxrandr-dev libxi-dev libboost-dev
    sudo apt-get install libglm-dev libglew-dev libopenimageio-dev libshmdata-0.8-dev libjsoncpp-dev libgsl0-dev libzmq3-dev libsnappy-dev
    sudo apt-get install libglfw3-dev libopenmesh-dev

    git clone git://github.com/paperManu/splash
    cd splash
    git checkout develop
    git submodule init && git submodule update
    ./autogen.sh && ./configure
    make && sudo make install

You can now try launching Splash:

    splash --help

<a name="architecture"/></a>
Software architecture
---------------------
As said earlier, Splash was made very modular to handle situations outside the scope of domes. Most of the objects can be created and configured from the configuration file (see next section for the syntax). The most important and useful objects are the following:

- *camera*: a virtual camera, corresponding to a given videoprojector. Its view is rendered in an offscreen buffer.
- *window*: a window is meant to be outputted on a given video output. Usually it is linked to a specific camera.
- *image*: a static image.
- *image_shmdata*: a video flow read from a shared memory.
- *mesh*: a mesh (and its UV mapping) corresponding to the projection surface, described as vertices and uv coordinates.
- *object*: utility class to specify which image will be mapped on which mesh.
- *gui*: a GUI, meaning that it is possible to launch Splash with no GUI if it is already configured.

The other, less useful but who knows objects, are usually created automatically when needed. It is still possible to create them by hand if you have some very specific needs (or if you are a control freak):

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
                "blendWidth" : 0.05,
                "shared" : 0
            },
            "win" : {"type" : "window", "fullscreen" : 1},
    
            "mesh" : {"type" : "mesh", "file" : "surface.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "image" : {"type" : "image", "file" : "color_map.png", "benchmark" : 0},
            "gui" : {"type" : "gui"},
    
            "links" : [
                ["mesh", "object"],
                ["image", "object"],
                ["object", "cam"],
                ["object", "gui"],
                ["cam", "win"],
                ["gui", "win"]
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
                "blendWidth" : 0.05,
                "shared" : 0
            },
            "win" : {"type" : "window", "fullscreen" : 1},
    
            "mesh" : {"type" : "mesh", "file" : "surface.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "image" : {"type" : "image", "file" : "color_map.png", "benchmark" : 0},
            "gui" : {"type" : "gui"},
    
            "links" : [
                ["mesh", "object"],
                ["image", "object"],
                ["object", "cam"],
                ["object", "gui"],
                ["cam", "win"],
                ["gui", "win"]
            ]
        }

This section describes the objects present in the *Scene* named "local". This *Scene* contains a single *camera*, linked to a *window*. An *object* consisting of a *mesh* on which an *image* is mapped is drawn through the *camera*. Also, a *gui* is set.

At the end of this section is a list of all the links between the objects. Note that the *object* is connected both to the *camera* and the *gui*: if it were only connected to the *camera*, it would not be visible in the *gui*.


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
                "blackLevel" : 0.0,
                "shared" : 0
            },
            "cam2" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0,
                "shared" : 0
            },
            "cam3" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0,
                "shared" : 0
            },
            "cam4" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, 2.0, -0.5],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "blackLevel" : 0.0,
                "shared" : 0
            },
            "win1" : {"type" : "window", "fullscreen" : 0},
            "win2" : {"type" : "window", "fullscreen" : 1},
            "win3" : {"type" : "window", "fullscreen" : 2},
            "win4" : {"type" : "window", "fullscreen" : 3},
    
            "mesh" : {"type" : "mesh", "file" : "sphere.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "shmimage" : {"type" : "image_shmdata", "file" : "/tmp/switcher_default_video_video-0"},
            "gui" : {"type" : "gui"},
    
            "links" : [
                ["mesh", "object"],
                ["object", "cam1"],
                ["object", "cam2"],
                ["object", "cam3"],
                ["object", "cam4"],
                ["object", "gui"],
                ["shmimage", "object"],
                ["cam1", "win1"],
                ["cam2", "win2"],
                ["cam3", "win3"],
                ["cam4", "win4"],
                ["gui", "win1"]
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
                "blendWidth" : 0.05,
                "shared" : 0
            },
            "cam2" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "shared" : 0
            },
            "win1" : {"type" : "window", "fullscreen" : 0},
            "win2" : {"type" : "window", "fullscreen" : 1},
    
            "mesh" : {"type" : "mesh", "file" : "sphere.obj"},
            "object" : {"type" : "object", "sideness" : 2},
            "shmimage" : {"type" : "image_shmdata", "file" : "/tmp/switcher_default_video_video-0"},
            "gui" : {"type" : "gui"},
    
            "links" : [
                ["mesh", "object"],
                ["object", "cam1"],
                ["object", "cam2"],
                ["object", "gui"],
                ["shmimage", "object"],
                ["cam1", "win1"],
                ["cam2", "win2"],
                ["gui", "win1"]
            ]
        },
    
        "second" : {
            "cam3" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [-2.0, -2.0, 0.3],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "shared" : 0
            },
            "cam4" : {
                "type" : "camera",
                "size" : [1920, 1200],
                "eye" : [2.0, 2.0, -0.5],
                "target" : [0.0, 0.0, 0.5],
                "blendWidth" : 0.05,
                "shared" : 0
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
                ["object", "gui"],
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

- eye [float, float, float]: position of the camera in the 3D space
- target [float, float, float]: vector indicating the direction of the camera view
- up [float, float, float]: vector of the up direction (used to tilt the view)
- fov [float]: field of view of the camera, in degrees
- size [int, int]: rendering size of the offscreen buffer. Will be updated when connected to a window
- principalPoint [float, float]: optical center of the camera, in image coordinates
- blendWidth [float]: width of the blending zone, along the camera borders
- blackLevel [float]: minimum value outputted by the camera
- brightness [float]: modify the global brightness of the rendering
- shared [int]: (deprecated) if set to 1, send the output through a shmdata

### geometry
Links from:

- mesh

Links to:

- object

Attributes: None

### gui
Links from:

- camera
- object

Links to:

- window

Attributes: None

### image
Links from: None

Links to:

- texture
- window

Attributes:

- benchmark [int]: if set to anything but 0, the *World* will send the image at every iteration even if the image has not been updated
- srgb [int]: if set to anything but 0, the image will be considered to be represented in the sRGB color space

### image_shmdata
As this class derives from the class image, it shares all its attributes and behaviors.

Links from: None

Links to:

- texture
- window

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

### texture
A texture handles an image buffer, be it static or dynamic.

Links from:

- image

Links to:

- object
- window

Attributes: None

### window
A window is an output to a screen.

Links from:

- camera
- gui
- image
- texture

Links to: None

Attributes:

- fullscreen [int]: if set to:
    - -1: disables fullscreen for this window
    - any positive or null integer: tries to set the window to fullscreen on the given screen number
- layout [int, int, int, int]: specifies how the input textures are ordered. This field is only needed if multiple cameras are connected to a single window, e.g. in the case of a window spanning through multiple projectors.
- swapInterval [int]: By default, the window swap interval is the same as the scene. If set to:
    - 0: disables any vSync
    - any positive integer: will wait for as many frames between each window update.

### world
The *World* is a single object created at launch by Splash. It handles all the inputs and configuration read / write, and dispatches messages and various buffers to all *Scenes*. It is not possible to add another *World* to a Splash session, although it is possible to specify a few attributes in the configuration file.

Links from: None

Links to: None

Attributes:

- framerate [int]: refresh rate of the *World* main loop. Setting a value higher than the display refresh rate may help reduce latency between video input and output.


<a name="calibration"/></a>
Calibration
-----------

This is a (very) preliminary guide to projector calibration in Splash, feel free to report any error or useful addition.

The way calibration works in Splash is to reproduce as closely as possible the parameters of the physical projectors onto a virtual camera. This includes its intrinsic parameters (field of view, shifts) as well as its extrinsic parameters (position and orientation). Roughly, to find these parameters we will set a few point - pixel pairs (at least six of them) and then ask Splash to find values which minimizes the squared sum of the differences between point projections and the associated pixel position.

Once the configuration file is set and each videoprojector has an output, do the following on the screen which has the GUI :

- Press 'tab' to make the GUI appear,
- click on the dome view which is in the upper right corner,
- press space to navigate through all the cameras, until you find the one you want to calibrage,
- press 'W' to switch the view to wireframe,
- left click on a vertex to select it,
- shift + left click to specify the position where this vertex should be projected. You can move this projection with the arrow keys.
- continue until you have six pairs of point - projection. You can orientate the view with the mouse (left click + drag) and zoom in / out with the wheel.
- press 'C' to ask for calibration.

At this point, you should have a first calibration. Pressing 'C' multiple times can help getting a better one, as is adding more pairs of point - projection. You can go back to previous calibration by pressing 'R'.

Once you are happy with the result, you can go back to textured rendering by pressing 'T' and save the configuration by pressing 'Ctrl' + 'S'. It is advised to save after each calibrated camera (you never know...). Also, be aware that the calibration points are not saved, and will be lost after reloading the configuration.


<a name="blending"/></a>
Blending
--------

This is an easy part, as long as calibration is done: with the GUI open, press 'B' and wait for it to be computed!

Although it is automatic, it is known to have some issues in specific cases which were not solved yet, but seem to be mostly related to the mesh. In particular, try replacing the mesh with a more / less refined one to check if it solves the issue. Note that the blending state is not saved yet.
