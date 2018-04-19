Splash release notes
===================

Splash 0.7.16 (2018-04-19)
--------------------------
Improvements:
- Upgraded ImGui
- Added a button to resend numeric attributes
- Improved cameras hide behavior
- Added CI for Ubuntu Bionic Beaver
- Improved automatic black value settings
- Refactored the configuration file
- Removed references to OSX from the project (not from the code, yet)
- Cleaned up sources
- Changed GUI font
- Updated --info option output
- Removed the window fullscreen attribute
- Improved calibration through GSL

Bugs fixed
- Fixed issue in Blender addon when exporting mesh_shmdata
- Fixed copy/paste
- Fixed FileSelector and its usage in Gui
- Fixed install target installing Snappy lib and headers
- Fixed compilation on Archlinux
- Fixed a GUI issue: default media when adding to playlist was different that the one displayed
- Fixed crash in ImageFFmpeg when a video file is not found
- Fixed a few issues related to color calibration
- Fixed Texture Image showing in the Media panel
- Fixed master clock, removed Timer dependencies to Value
- Fixed auto black level behavior
- Fixed media panel closing unexpectedly
- Fixed a random segfault in Python callbacks
- Fixed issue when pausing a video
- Fixed project loading when multiple scenes are used
- Fixes in documentation and help

Splash 0.7.14 (2018-02-05)
--------------------------
New features:
- Added the trim attribute to image_ffmpeg                                           
- Added a texture debugging panel                                                    

Improvements:
- Added a Python module search path to Blender addon
- Factory registers objects' savability                                              
- Updated docs                                                                       

Bugs fixed:
- Fixed issues with calibration controls                                             
- Fixed object linked to cameras when loading project                                

Splash 0.7.12 (2018-01-05)
--------------------------
New features:
- Added support for named values, and a mediaInfo attribute to Images

Improvements:
- Added Python as a dependency for CI
- Bundled GL FBOs into the Framebuffer class
- Added get_size() method to Python sink
- Added splash.get_interpreter_name() to Python
- Updated ZMQ and Hap

Bugs fixed:
- Fixed blending when a virtual probe is in use, with multiple GPUs
- Fixed a GPU leak in ProfilerGL
- Fixed issue with command line args

Splash 0.7.10 (2017-12-04)
-------------------------
New features:
- Added -P option to set python script from args                                                                        
- OpenGL performance profiler.                                                                                          
- Ease-of-use for performance profiler flamegraph generation.
- Added conditional profiling of render loop and texture upload

Improvements:
- Updated FFmpeg and Snappy                                  
- Some other Python Sink fixes to make it more reliable                                        
- Removed the splash-scene executable, replaced by an option on splash                                                  
- Added a gui checkbox to activate / deactivate 16bits rendering for all cameras                                        

Bugs fixed:
- Fixes following static analysis                                                                                       
- Hardened Python Sink wrapper                                                                                          
- Bool to int cast caused a crash in python binding. (Hint: don't do it)                                                
- Fixed issues related to GCC 7.2 and Ubuntu 17.10                                                                      

Splash 0.7.8 (2017-10-20)
-------------------------
New features:
- Added method to get master clock from Python
- Added a keepRatio attribute to Filter, and keep_ratio(bool) method to Python sink
- Added python bindings to get an object's type and all objects of a type.

Improvements:
- Every object now has a task queue
- Improved help for Python Sink object
- Replaced SSE2 detection with x86-64 detection
- Specified memory ordering for important atomic operations

Bugs fixed:
- Temporarily fixed banding in dark regions
- Fixed issue with blending happening on calibration markers
- Image_V4L2: fixed a deadlock issue with automatic resizing
- Fixed issue with multiple textures linked to a single image
- Fix: keep relative paths relative

Splash 0.7.6 (2017-09-28)
-------------------------
New features:
- Added a virtual screen object
- Added a loop rate parameter to the Python interpreter
- Hidden the Python splash capsule in the splash module
- Added a Sink wrapper to the Python interpreter
- Added attribute callbacks
- Simulate swap synchro when splash is hidden to avoid taking 100% GPU.

Improvements:
- Added a Probe node to the Blender addon
- Removed ghost-specific commands
- Moved swap interval set from Scene to Window
- Duplicated the link and unlink commands in the World, to simplify the API
- Renamed controller methods
- Added ZMQ as a submodule                
- Added splash.get_timings() to Python API
- Deleted the thread pool, replaced with futures or mere threads.

Bugs fixed:
- Fixed an issue in the Blender exporter, when exporting only the 3D models
- Fixed issue with warps 
- Fixed factory not creating Python controller
- Fixed a #ifdef issue in image_shmdata 
- Fixed Python controller object not loading correctly custom attributes
- Fixed adaptive framerate
- Fixed Python custom attributes
- Fixed integration tests
- Fixed issue with ImGui style editor not building

Splash 0.7.4 (2017-07-31)
-------------------------
New features:
- Added multisampling to camera rendering
- Added anisotropic filtering to Texture_Image

Improvements:
- GUI visible by default when on its own window

Bug fixed:
- Controller_Gui: fixed clear issue when resizing
- Fixed copy parameters not copying warps


Splash 0.7.2 (2017-07-10)
-------------------------
Improvements:
- Added the reprojection error in the gui
- Updated 2D marker to show the up direction
- Texture upload thread destruction is now at the same place as its creation

Bug fixed:
- Fixed stopping v4l2 capture
- Fixed live calibration update

Splash 0.7.0 (2017-06-29)
-------------------------
New features:
- Added --prefix option, to add a prefix to shared memories

Improvements:
- Updated ImGui version
- Cleaned up OpenGL context switching
- Added video codecs integration tests
- GL cleanup - removed non useful state changes
- Calibration points targets are now shown by default

Bug fixed:
- Fixed issue with Sink_Shmdata_Encoded reconnecting ad vitam
- Fixed warp rednering jitter when editing control points
- Fixed a multi-GPU issue
- Fixed GLSL YUV to RGB conversion

Splash 0.6.8 (2017-04-28)
-------------------------
New features:
- Blender: World node can export only 3D models
- Added uninstall target
- Added a splash / about screen

Improvements:
- Can now set any display identifier
- Improved buffer updated signaling

Bug fixed:
- Fixed issue with Hap and HapAlpha format
- Partially fixed OSX installation instructions
- Fixed a random freeze when exiting while a video is playing
- Fixed throwing error at exit
- Fixed clang compilation error on OSX

Splash 0.6.6 (2017-04-28)
-------------------------
Improvements:
- Converted to OpenGL 4.5
- Improved the file selector
- Sinks now use framerate as a parameter (instead of period), shmdata sinks fill the caps with this parameter
- Moving mouse over a window which does not hold the GUI does not move the mouse on the GUI window anymore

Bug fixed:
- Fixed warp jittery movement. Still some undesired visual glitches when editing warps
- Fixed window fullscreen behavior

Splash 0.6.4 (2017-04-13)
-------------------------
New features:
- Added a desktop entry

Improvements:
- Some code cleanup, refactored some base classes
- Added object description to the output of splash --info
- Filter: output size can now be overridden

Bug fixed:
- Fixed installation instructions
- Fixed Debian package

Splash 0.6.2 (2017-03-30)
-------------------------
New features:
* Added automatic black level to filters

Improvements:
* Sinks can now be opened/closed at will
* Can now limit sinks rate, given a period between frames
* GL version and swap group status are now read from getters

Bug fixed:
* Fixed issues with distant attributes not showing in the controls panel
* Fixed NVSwapGroups

Splash 0.6.0 (2017-03-19)
-------------------------
New features:
* Added a looseClock parameter to control master clock effect on media
* Added a --hide argument for running Splash in background
* Added Sink_Shmdata_Encoded
* Added the possibility to set default values through a configuration file

Improvements:
* Snappy is now included as a submodule
* Image_V4L2: Removed capture rate parameter, which is unused and leads to issues
* Forcing a (working) version number for cppcheck
* Various improvements following static analysis results

Bug fixed:
* Blender addon: exporting a configuration exists edit mdoe (thus preventing it from crashing)
* Fixed issues with loading image with non-standard size
* Fixed blending issue when loading file with blending activated
* Fixed issue where updating an image with another which has the same resolution needed to be done twice
* Fixed texture format issue
* Fixed issue with --info
* Fixed loose clock behavior when no master clock has been received yet; also, GUI better reflect the loose clock setting

Splash 0.5.8 (2017-03-02)
-------------------------
New features:
* Added support for RGB correction curves in Filter
* Added a static_analysis target, using cppcheck. Also added cppcheck t o gitlab CI
* Added a freerun parameter to media in queue

Improvements:
* Small performance improvements
* Splash now exits with an error if the default 3D models can not be found
* Attributes are now ordered in the GUI
* Refactoring/cleanup of Texture_Image
* Filter: comments places right after uniforms declaration are used as comments in the GUI
* Removed _setMutex, as it was replaced with synced tasks(with addTask)

Bug fixed:
* Fixed various random issues in Image_FFmpeg
* Fixed issue with PNG saving
* Fixed build on OSX

Splash 0.5.6 (2017-02-10)
-------------------------
New features:
* Refactored ImageBufferSpec to add support for YUV (and other formats). Updated other classes to reflect this change

Improvements:
* Added _tex#_size uniform for Texture_Images
* Blender addon can now export a project (and not the whole configuration)
* Refactored ImageBufferSpec to add support for YUV (and other formats). Updated other classes to reflect this change

Bug fixed:
* Fixed videos looping before the end
* Fixed issues with planar audio having only the first channel decoded
* Fixed audio sync
* Fixed GUI creating a Python interpreter in every Scene: only the master Scene is worthy
* Fixed a segfault when quitting
* Fixed partly deadlocks occuring when modifying heavily the configuration at runtime

Splash 0.5.4 (2017-01-30)
-------------------------
New features:
* Added a Mesh gui panel
* Added a Sink base class, and Sink_Shmdata
* Added a Sound_Engine class, Speaker can now output to Jack
* Added the possibility to visually show the camera count
* Added a priorityShift attribute to BaseObject

Improvements:
* Timestamps now contain the date
* Added GLFW as a submodule, statically linked
* Filter now accepts files as shaders
* Now using spinlocks where it makes sense
* Improved Filter contrast and brightness settings
* Added recurring tasks to RootObjects; moved swapTest and pingTest to recurring tasks
* Filter now accepts multiple inputs

Bug fixed:
* Fixed jitter while reading with a queue
* Fixed a crash when creating a texture_image from the gui
* Fixed CI, removed Travis support

Splash 0.5.2 (2017-01-13)
--------------------------
New features:
* Added Video4Linux2 support, as well as Datapath capture cards support
* Added _time (in ms) as a built-in uniform for Filter
* Added --forceDisplay command line option

Splash 0.5.0 (2016-12-09)
--------------------------
New features:
* If possible, Splash now writes logs to /var/log/splash.log
* Added access to logs through Python
* Added get_object_description and get_type_from_category methods to Python module
* Added Debian package build through Docker

Improvements:
* Embedding FFmpeg and link statically to it
* Resetting the up vector when moving the camera manually
* Generates automatically package through Gitlab artifacts
* Objects now have a Class, and can be listed by it by the factory

Bug fixed:
* Fixed an issue with node view
* Fixed compilation issue on GCC 4.8
* Fixed a bug where configuration parameters had to be arrays, even for single values

Splash 0.4.14 (2016-11-27)
--------------------------
New features:
* Added integration testing using Python
* Objects now ask the configuration path from their root; Added a media path

Improvements:
* BufferObjects now force a World loop iteration, to improve sync
* Fixed an issue in world loop, which was sending empty serialized buffers when the objects were not updated
* Image_FFmpeg: forcing decoding thread_count, works depending on the codec
* Image_FFmpeg: getting video codec info and setting it as an attribute
* Limited threadpool size to 32 instead of 16
* Disable input texture filtering by default, as the Filter class will also create mipmaps

Bug fixed:
* Fixed an issue with blending computation at startup
* Fixed an issue when trying to replace an object with a non-existing type
* Integration test used installed binary instead of local one
* Fixed an issue with FileSelector, which reverted to run path after the first use
* Image_FFmpeg: fixed read issue with some video codecs

Splash 0.4.12 (2016-11-11)
--------------------------
Improvements:
* Added the possibility to save a project, which holds only media (objects and images), and leaves projectors projectors calibration as is
* Image_FFmpeg: can now set a buffer size limit for the frame queue
* Filter: added an invertChannel attribute, allowing for inverting R and B

Bug fixed:
* Fixed GPU sync issue
* Fixed an issue when resetting the clock device name to an empty string

Splash 0.4.10 (2016-10-28)
--------------------------
Improvements:
* Separated RT scheduling and core affinity options
* Added a template realtime.conf to be put in /etc/security/limits.d
* Cleaned up the threadpool
* Updated shmdata dependency to version 1.3

Bug fixed:
* Fix: issue when deleting an image_shmdata object
* Fix: texture upload loop crash when deleting a texture object
* Fix: freeze of the engine (not the gui) when saving configuration while having a camera selected in the calibration view
* Fix: blending crash issue when using multiple GPUs
* Fix: Controller::getObjectTypes returned the local type, instead of the remote type
* Fix: localhost address is now the default option when none is specified.
* Fix: updated default spawn value

Splash 0.4.8 (2016-09-30)
-------------------------
New features:
* Refactored user inputs, now supports drag'n'drop in path fields and copy/paste

Improvements:
* Refactored render loop, to make it more generic...
* ... which led to unexpected performance improvements.
* Cleaned up unit tests.
* Fixed most FFmpeg deprecation warning
* Fixed issues with loading configuration during runtime
* Some code and shader cleanup

Bug fixed:
* Issues in ResizableArray

Splash 0.4.6 (2016-09-02)
-------------------------
New features:
* Added Doxygen documentation
* Added continuous integration through Travis and Coverity
* Filters now accept user-defined fragment shader
* Objects can use a second texture as a mask

Improvements:
* Added hints for realtime and cpu affinity
* Blending is now handling by a Blender class, not by the Scene
* Removed old blending through a map method

Bug fixed:
* Fixed blending on Intel / Mesa12.1
* Fixed Blender addon version update on OSX

Splash 0.4.4 (2016-08-05)
-------------------------
New features:
* Scriptable with Python

Improvements:
* Improved vertex blending
* Wireframe coloring when in calibration mode, to help while matching projections
* New GUI font: OpenSans
* Fullsize GUI when alone in a window
* Cleaned Blender nodes
* Cleaned Shmdata objects
* Added a Controller base class, used in particular for Python scripting
* Removed Autotools
* Removed the HttpServer class, as it can now be done through Python
* Added a simple Python script implementing a HTTP server

Bug fixed:
* Serialization of Geometry
* Random crash in the main loop when sending a serialized object
* Object deletion was not saved

Splash 0.4.2 (2016-06-21)
-------------------------
New features:
* Added automatic documentation generation from in-code attribute description

Improvements:
* Improved and fixed node view
* Cleaned up mutexes
* Can now rename objects from the GUI

Bug fixed:
* RGB and BGR were inverse in Image_Shmdata
* Fixed window moving after switching decorations

Splash 0.4.0 (2016-06-07)
-------------------------
New features:
* Added CMake as a possible build system. Keeping autotools for now.

Improvements:
* Improved warp GUI
* Improved wireframe rendering
* Improved attributes: added attribute description, can now lock them temporarily
* Messages can now be handling asynchronously in world and scene
* Added GLM as a submodule
* Added limits to calibration
* Queue: auto-complete media duration
* Improve configuration save, now saving world parameters

Bug fixed:
* Fixed a bug in camera calibration
* Fixed issues with window layout
* Fixed an issue with FFmpeg 3.x
* Fixed an sync issue with vertex blending on multiple GPUs
* Fixed application exit code

Splash 0.3.12 (2016-03-25)
-------------------------
New features:
* Added warp to take care of thoses cases where camera calibration is not sufficient
* Added per-media filter, to control brightness, contrast, saturation and black level
* Added support for threaded HapQ decoding

Improvements:
* Improved performances by reducing the number of buffer copies
* Improved queues
* Improved LTC support
* Added a file selector for media and configurations
* Overall GUI improvements
* Removed dependency to OpenImageIO, now using stb_image
* JsonCpp, libltc and cppzmq are now included as submodules

Bug fixed:
* Fixed some crashes related to $DISPLAY
* Fixed a crash specific to OSX

Splash 0.3.10 (2015-12-08)
-------------------------
Improvements:
* Improved GUI to hide complexity

Bug fixed:
* Fixed a random race condition in Timer
* Fixed a crash happening on Object deletion

Splash 0.3.8 (2015-11-22)
-------------------------
Improvements:
* Big improvements to the Blender addon

Splash 0.3.6 (2015-11-13)
-------------------------
New features:
* Added queues, to create playlists of media
* SMTPE LTC can be activated for compatible sources (image_ffmpeg and queues)
* Added a preliminary support for joysticks during calibration

Improvements:
* If possible, no additional process is launched for a Scene
* Improved sound synchronization
* HttpServer can control Scene parameters
* Improved Blender addon
* Improved logs and timing handling

Bug fixed:
* Fixed a race condition at launch time
* Fixed random freezes when quitting

Splash 0.3.4 (2015-09-30)
-------------------------
New features:
* Sound support in Image_FFmpeg
* Seeking in Image_FFmpeg
* Added architecture for effects on video
* SMPTE LTC support, though not active yet

Improvements:
* Various fixes to enable building on OSX / Homebrew
* Now using GLAD as a GL loader
* Improved HTTP server

Bug fixed:
* Camera calibration

Splash 0.3.2 (2015-08-05)
-------------------------
New features:
* Real-time blending (for OpenGL >= 4.3)
* Added an HTTP server (not activated by default, draft version)

Improvements:
* More control over the configuration from the GUI
* Slight performance improvements
* Automatic GL version detection
* Improved Blender addon

Bug fixed:
* Keyboard shortcuts
* Memory leak in Blender addon

Splash 0.3.0 (2015-05-25)
-------------------------
New features:
* Packaged for OSX!
* Added support for Syphon on OSX
* Can read videos through ffmpeg
* Can access video grabbers through OpenCV
* Added an exporter for Blender to ease configuration
* Supports multithreaded Hap

Improvements:
* Improved camera manipulation
* Improved GUI
* Removed dependency to OpenMesh
* Now uses shmdata 1.0
* Various optimizations and fixes

Bug fixed:
* Fixed shaders to work on ATI cards
* Fixed some crashes

Splash 0.2.2 (2015-02-27)
-------------------------
New features:
* New GUI, more complete and user friendly, based in ImGui (removed GLV)

Improvements:
* Fully OpenGL 3 as GLV has been removed. Less context switching.

Bug fixed:
* A bug in color calibration prevented it to work in some cases

Splash 0.2.0 (2015-02-18)
-------------------------
New features:
* First version of color and luminance automatic calibration through a camera
* Support for Y422 / UYVY video feed
* Blender addon to feed Splash

Improvements:
* More flexibility for window positioning
* Overall performance improvements
* Better blending map computation
* Calibration points are saved (along calibration itself)

Bug fixed:
* Now needs 7 points for calibration (6 was not enough in many cases)
* Various bug fixes and code cleanup

Splash 0.1.8 (2014-11-17)
-------------------------
New features:
* NV swap group support
* Mesh update through shared memory (shmdata)
* Flip / flop for input textures
* Color temperature correction

Improvements:
* Slight improvement of blending map computation
* Camera movements now similar to Blender
* Cleaned up header files
* Updated GL debug filters (disabled by default)

Bug fixed:
* Concurrency issue in Link
* Warnings related to GLM
* Cleanup to compile on Ubuntu 14.04
* Corrected performance issue in Threadpool
* Corrected Threadpool issue with cores number detection
* Channel inversion for RGBA input

Splash 0.1.6 (2014-07-07)
-------------------------
New features:
* HapQ video codec supported

Various bug fixes and improvements

Splash 0.1.4 (2014-05-29)
-------------------------
New improvements:
* Support of 4K @ 30Hz
* GLSL: better uniforms management

Splash 0.1.2 (2014-05-02)
-------------------------
New features:
* Overall performance improvements
* Calibration is now done in realtime
* Removed dependency to OpenCV

Bug fixed:
* Fixed a random crash happening roughly once a day
* Fixed alignment issues in SSE YUV to RGB conversion
* Fixed a bug when two video sources where configured

New Documentation:
* Added a first documentation (README.md)

Splash 0.1.0 (2014-04-06)
-------------------------
First release of Splash

New features:
* Can be used for real
