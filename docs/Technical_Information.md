Technical information
=====================

## Software architecture
As said earlier, Splash was made very modular to handle situations outside the scope of domes. The most important and useful objects are the following:

- *camera*: a virtual camera, corresponding to a given videoprojector. Its view is rendered in an offscreen buffer.
- *window*: a window is meant to be outputted on a given video output. Usually it is linked to a specific camera.
- *images*: image source, be it static or a video
- *filter*: filters applied to images. The default filter gives standard control over the input, the user can specify his own GLSL shader (see [Filters](../Advanced_usages/#glsl-filter-shaders))
- *mesh*: a mesh (and its UV mapping) corresponding to the projection surface, described as vertices and uv coordinates.
- *object*: utility class to specify which image will be mapped on which mesh.
- *python*: controller based on a Python script.
- *queue*: allows for creating a timed playlist of video sources.

The other, less useful (but who knows) objects, are usually created automatically when needed. It is still possible to create them by hand if you have some very specific needs (or if you are a control freak):

- *geometry*: intermediary class holding vertex, uv and normal coordinates of a projection surface, to send them to the GPU.
- *shader*: a shader, describing how an object will be drawn.
- *texture*: a texture handles the GPU memory in which image frames are copied.

These various objects are connected together through *links*, which you have to specify (except for automatically created objects obviously). They are managed in the *World* object for all objects which are not related to OpenGL, and in the various *Scene* objects for all the other ones (although non-GL objects are somewhat mirrored in *Scenes*). In a classic configuration, there is one *World* and as many *Scenes* as there are graphic cards. *World* and *Scenes* run in their own process and communicate through shared memories (for now). They derive from the same base object, *Root*

The global state of the *World*, *Scenes* and the objects is stored in a *Tree*. Each *Root* object has a instance of the *Tree*, all these instances being synchronized between them. Any update to an attribute in the *Tree* is duplicated to all *Tree* instances and propagated to the corresponding object or *Root*. Conversely, any update to an attribute in an object is propagated to the *Trees*. The structure of the *Tree* is described in the next section.

### Tree structure

The *Tree* structure follows the structure of the various processes ran by Splash. The first level of the tree represents the *Root* objects, each of them have attributes, objects, etc. The following structure is a good example of the *Tree* used in Splash:

```
Root
  |- world
  | |- attributes
  | |- commands
  | |- logs
  | |- durations
  | |- objects
  |- scene_1
  | |- attributes
  | | |- attribute_1
  | | |- ...
  | |- commands
  | |- logs
  | |- durations
  | |- objects
  |   |- object_1
  |   | |- links
  |   | | |- children
  |   | | | |- object_2
  |   | | | |- ...
  |   | | |- parents
  |   | | | |- object_3
  |   | | | |- ..
  |   | |- attributes
  |   | | |- attribute_1
  |   | | |- attribute_2
  |   | | |- ...
  |   | |- documentation
  |   |   |- attribute_1_documentation
  |   |   |- attribute_2_documentation
  |   |   |- ...
  |   |- object_2
  |   | |- ...
  |   |- object_3
  |   | |- ...
  |   |- ...
  |- scene_1
    |- attributes
    |- commands
    |- logs
    |- durations
    |- objects
```

### Objects description

This section describes the object types which are of use to the final user. Some other types exist but are meant to be used internally or for testing purposes. You can get a description of all objects and their attributes by running the following command:

```bash
splash -i

# To get a mardown version of the output, run from the source root path:
splash -i | ./tools/info2md.py
```


## Performance profiling

A header-only profiler has been implemented to measure the time spent in OpenGL operations.
It supports multiple threads and uses a RAII implementation to ease its use. If you want to profile a block of code 
just include the header include/profilerGL.h into your source and call
```
PROFILE("My scope name")
```
every time you want to measure the time spent by the graphics driver in nanoseconds in the current scope.
If you use PROFILE blocks in multiple threads they will be separated appropriately in the processed data. The 
hierarchy of execution scopes is also preserved so as to know which proportion of the time is spent in the current 
scope or in its underlying profiled scopes. All the processing not profiled will be accounted for in the duration of
 the directly higher profiled scope.

Currently, only one format of display is supported: [FlameGraph](https://github.com/brendangregg/FlameGraph). 
Whenever you want to process the profiling data you need to call:

```cpp
ProfilerGL::get().processTimings();
```

This will preprocess the data to sort through the cumulated timings in all profiled scopes. After that to postprocess
 the data for FlameGraph:

```cpp
ProfilerGL::get().processFlamegraph();
```

If you want to flush the data at this point:

```cpp
ProfilerGL::get().clearTimings();
```

Once you have done that just use the FlameGraph to create the browsable graph:

```cpp
flamegraph.pl /tmp/splash_profiling_data_${SCENE_NAME} > /tmp/profiling_data.svg
```

And open the svg with your browser.

To activate profiling, Splash must be compiled with the PROFILE_OPENGL flag activated.
As of yet, there is no way to view the profiling information in the Splash GUI.
