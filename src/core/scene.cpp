#include "./core/scene.h"

#include <list>
#include <utility>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "./config.h"
#include "./controller/controller_blender.h"
#include "./controller/controller_gui.h"
#include "./core/constants.h"
#include "./graphics/camera.h"
#include "./graphics/filter.h"
#include "./graphics/geometry.h"
#include "./graphics/object.h"
#include "./graphics/profiler_gl.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/warp.h"
#include "./graphics/window.h"
#include "./image/image.h"
#include "./image/queue.h"
#include "./mesh/mesh.h"
#include "./network/link.h"
#include "./userinput/userinput_dragndrop.h"
#include "./userinput/userinput_joystick.h"
#include "./userinput/userinput_keyboard.h"
#include "./userinput/userinput_mouse.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/scope_guard.h"
#include "./utils/timer.h"

#if HAVE_GPHOTO and HAVE_OPENCV
#include "./controller/colorcalibrator.h"
#endif

#if HAVE_LINUX
// clang-format off
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#include <GL/glxext.h>
// clang-format on
#endif

#if HAVE_CALIMIRO
#include "./controller/geometriccalibrator.h"
#include "./controller/texcoordgenerator.h"
#endif

namespace chrono = std::chrono;

namespace Splash
{

std::vector<std::string> Scene::_ghostableTypes{"camera", "warp"};

/*************/
Scene::Scene(Context context)
    : RootObject(context)
{
    _type = "scene";
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Scene::Scene - Scene created successfully" << Log::endl;
#endif

    _isRunning = true;
    _name = _context.childSceneName;

    registerAttributes();
    initializeTree();

    // We have to reset the factory to create a Scene factory
    _factory = std::make_unique<Factory>(this);
    _blender = std::make_shared<Blender>(this);
    if (_blender)
    {
        _blender->setName("blender");
        _objects["blender"] = _blender;
    }

    init(_name);

    // Create the link and connect to the World
    _link = std::make_unique<Link>(this, _name, _context.channelType);

    if (!_link->connectTo("world"))
    {
        Log::get() << Log::ERROR << "Scene::Scene - Could not connect to world from scene " << _name << Log::endl;
        _isRunning = false;
    }
}

/*************/
Scene::~Scene()
{
    // No renderer, probably means we failed to init anyway so no clean up is needed.
    if (_renderer)
    {
        if (const auto mainRenderingContext = _renderer->getMainContext(); mainRenderingContext)
        {
            mainRenderingContext->setAsCurrentContext();
            std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex); // We don't want any friend to try accessing the objects

            // Free objects manually
            for (auto& obj : _objects)
                obj.second.reset();
            _objects.clear();

            mainRenderingContext->releaseContext();
        }
    }

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Scene::~Scene - Destructor" << Log::endl;
#endif
}

/*************/
std::shared_ptr<GraphObject> Scene::addObject(const std::string& type, const std::string& name)
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - Creating object of type " << type << Log::endl;
#endif

    std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

    // If we run in background mode, don't create any window
    if (_runInBackground && type == "window")
        return {};

    // Check whether an object of this name already exists
    if (getObject(name))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - An object named " << name << " already exists" << Log::endl;
#endif
        return {};
    }

    // Create the wanted object
    auto obj = _factory->create(type);

    // Add the object to the objects list
    if (obj.get() != nullptr)
    {
        obj->setRemoteType(type); // Not all objects have remote types, but this doesn't harm

        obj->setName(name);
        _objects[name] = obj;

        // Some objects have to be connected to the gui (if the Scene is master)
        if (_gui != nullptr)
        {
            if (obj->getType() == "object")
                link(obj, std::dynamic_pointer_cast<GraphObject>(_gui));
        }
    }

    return obj;
}

/*************/
void Scene::addGhost(const std::string& type, const std::string& name)
{
    if (find(_ghostableTypes.begin(), _ghostableTypes.end(), type) == _ghostableTypes.end())
        return;

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - Creating ghost object of type " << type << Log::endl;
#endif
    auto obj = addObject(type, name);
    if (obj)
    {
        auto ghostPath = "/" + _name + "/objects/" + name + "/ghost";
        _tree.createLeafAt(ghostPath);
        _tree.setValueForLeafAt(ghostPath, true);
    }
}

/*************/
bool Scene::link(const std::string& first, const std::string& second)
{
    return link(getObject(first), getObject(second));
}

/*************/
bool Scene::link(const std::shared_ptr<GraphObject>& first, const std::shared_ptr<GraphObject>& second)
{
    if (!first || !second)
        return false;

    std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);
    bool result = second->linkTo(first);

    return result;
}

/*************/
void Scene::unlink(const std::string& first, const std::string& second)
{
    unlink(getObject(first), getObject(second));
}

/*************/
void Scene::unlink(const std::shared_ptr<GraphObject>& first, const std::shared_ptr<GraphObject>& second)
{
    if (!first || !second)
        return;

    second->unlinkFrom(first);
}

/*************/
void Scene::setEnableJoystickInput(bool enable)
{
    static std::string joystickName = "joystick";
    if (!_joystick && enable)
    {
        _joystick = std::make_shared<Joystick>(this);
        _joystick->setName(joystickName);
        _objects[_joystick->getName()] = _joystick;
    }
    else if (_joystick && !enable)
    {
        _joystick.reset();
        if (auto objectsIt = _objects.find(joystickName); objectsIt != _objects.end())
            _objects.erase(objectsIt);
    }
}

/*************/
bool Scene::getEnableJoystickInput() const
{
    return _joystick != nullptr;
}

/*************/
void Scene::remove(const std::string& name)
{
    std::shared_ptr<GraphObject> obj;
    std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

    if (_objects.find(name) != _objects.end())
        _objects.erase(name);
}

/*************/
void Scene::render()
{
    PROFILEGL(Constants::GL_TIMING_TIME_PER_FRAME);

    // We want to have as much time as possible for uploading the textures,
    // so we start it right now.
    bool expectedAtomicValue = false;
    if (!_doUploadTextures.compare_exchange_strong(expectedAtomicValue, false, std::memory_order_acq_rel))
    {
        ZoneScopedN("Upload textures");

        Timer::get() << "textureUpload";
        std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);
        for (auto& obj : _objects)
        {
            auto texture = std::dynamic_pointer_cast<Texture>(obj.second);
            if (texture)
            {
                ZoneScopedN("Uploading a texture");
                ZoneName(obj.first.c_str(), obj.first.size());

                texture->update();
            }
        }
        Timer::get() >> "textureUpload";
    }

    {
        ZoneScopedN("Scene rendering");

        // Create lists of objects to update and to render
        std::map<GraphObject::Priority, std::vector<std::shared_ptr<GraphObject>>> objectList{};
        {
            ZoneScopedN("Preprocessing");

            std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);
            for (auto obj = _objects.cbegin(); obj != _objects.cend(); ++obj)
            {
                if (_isMaster)
                {
                    // If the object is a ghost from another scene, it should
                    // not be rendered in the main rendering loop. For example ghost
                    // Cameras are rendered by the GUI whenever they are shown
                    Value isGhost;
                    if (_tree.getValueForLeafAt("/" + _name + "/objects/" + obj->second->getName() + "/ghost", isGhost))
                    {
                        if (isGhost.as<bool>())
                            continue;
                    }
                }

                // We also run all pending tasks for every object
                obj->second->runTasks();

                auto priority = obj->second->getRenderingPriority();
                if (priority == GraphObject::Priority::NO_RENDER)
                    continue;

                auto listIt = objectList.find(priority);
                if (listIt == objectList.end())
                {
                    auto entry = objectList.emplace(std::make_pair(priority, std::vector<std::shared_ptr<GraphObject>>()));
                    if (entry.second == true)
                        listIt = entry.first;
                    else
                        continue;
                }

                listIt->second.push_back(obj->second);
            }
        }

        // Update and render the objects
        // See GraphObject::getRenderingPriority() for precision about priorities
        for (const auto& objPriority : objectList)
        {
            ZoneScopedN("Render bin");

            if (objPriority.second.size() != 0)
            {
                const auto type = objPriority.second[0]->getType();
                Timer::get() << type;
                ZoneName(type.c_str(), type.size());
            }

            for (auto& obj : objPriority.second)
            {
                ZoneScopedN("Rendering an object");
                ZoneName(obj->getName().c_str(), obj->getName().size());

                obj->update();

                auto objectCategory = obj->getCategory();
                if (objectCategory == GraphObject::Category::MESH)
                    if (obj->wasUpdated())
                    {
                        // If a mesh has been updated, force blending update
                        addTask([this]() { std::dynamic_pointer_cast<Blender>(_blender)->forceUpdate(); });
                        obj->setNotUpdated();
                    }
                if (objectCategory == GraphObject::Category::IMAGE || objectCategory == GraphObject::Category::TEXTURE)
                    if (obj->wasUpdated())
                        obj->setNotUpdated();

                obj->render();
            }

            if (objPriority.second.size() != 0)
                Timer::get() >> objPriority.second[0]->getType();
        }
    }

    {
        ZoneScopedN("Swap");

        // Swap all buffers at once
        Timer::get() << "swap";
        for (auto& obj : _objects)
            if (obj.second->getType() == "window")
                std::dynamic_pointer_cast<Window>(obj.second)->swapBuffers();
        Timer::get() >> "swap";
    }

    TracyGpuCollect;

    ProfilerGL::get().gatherTimings();
    ProfilerGL::get().processTimings();
    const auto glTimings = ProfilerGL::get().getTimings();
    for (const auto& threadTimings : glTimings)
        for (const auto& glTiming : threadTimings.second)
            Timer::get().setDuration(Constants::GL_TIMING_PREFIX + glTiming.getScope(), glTiming.getDuration() / 1000.0);
    ProfilerGL::get().clearTimings();
}

/*************/
void Scene::run()
{
    tracy::SetThreadName("Scene");

    if (!_isInitialized)
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - No rendering context has been created" << Log::endl;
        return;
    }

    auto mainRenderingContext = _renderer->getMainContext();
    mainRenderingContext->setAsCurrentContext();
    TracyGpuContext;

    while (_isRunning)
    {
        FrameMarkStart("Scene");
        ZoneScopedN("Main loop");
        ZoneName(_name.c_str(), _name.size());

        {
            // Process tree updates
            ZoneScopedN("Process tree");
            Timer::get() << "tree_process";
            _tree.processQueue();
            Timer::get() >> "tree_process";

            // Execute waiting tasks
            executeTreeCommands();
            runTasks();
        }

        // This gets the whole loop duration
        if (_runInBackground && _swapInterval != 0)
        {
            // Artificial synchronization to avoid overloading the GPU in hidden mode
            Timer::get() >> _targetFrameDuration >> "swap_sync";
            Timer::get() << "swap_sync";
        }

        Timer::get() >> "loop_scene";
        Timer::get() << "loop_scene";

        if (_started)
        {
            {
                ZoneScopedN("Render");
                Timer::get() << "rendering";
                render();
                Timer::get() >> "rendering";
            }

            {
                ZoneScopedN("Update inputs");
                Timer::get() << "inputsUpdate";
                updateInputs();
                Timer::get() >> "inputsUpdate";
            }
        }
        else
        {
            std::this_thread::sleep_for(chrono::milliseconds(50));
        }

        {
            ZoneScopedN("Propagate tree");
            Timer::get() << "tree_update";
            updateTreeFromObjects();
            Timer::get() >> "tree_update";
            Timer::get() << "tree_propagate";
            propagateTree();
            Timer::get() >> "tree_propagate";
        }

        // If no sync message was received from the World for more than 10 seconds, exit
        const int64_t syncTimeout = 10; // in seconds
        if (_lastSyncMessageDate != 0 && (Timer::getTime() - _lastSyncMessageDate) > 10 * 1e6)
        {
            Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - No sign of life from the main process for more than " << syncTimeout << " seconds, exiting." << Log::endl;
            _isRunning = false;
        }

        FrameMarkEnd("Scene");
    }
    mainRenderingContext->releaseContext();

    signalBufferObjectUpdated();

    // Clean the tree from anything related to this Scene
    _tree.cutBranchAt("/" + _name);
    propagateTree();

#ifdef PROFILE_GPU
    ProfilerGL::get().processTimings();
    ProfilerGL::get().processFlamegraph("/tmp/splash_profiling_data_" + _name);
#endif
}

/*************/
void Scene::updateInputs()
{
    glfwPollEvents();

    auto keyboard = std::dynamic_pointer_cast<Keyboard>(_keyboard);
    if (keyboard)
        _gui->setKeyboardState(keyboard->getState(_name));

    auto mouse = std::dynamic_pointer_cast<Mouse>(_mouse);
    if (mouse)
        _gui->setMouseState(mouse->getState(_name));

    // Check if we should quit.
    if (Window::getQuitFlag())
        sendMessageToWorld("quit");
}

/*************/
void Scene::setAsMaster(const std::string& configFilePath)
{
    std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

    _isMaster = true;

    _gui = std::make_shared<Gui>(_renderer->getMainContext(), this);
    if (_gui)
    {
        _gui->setName("gui");
        _gui->setConfigFilePath(configFilePath);
        _objects["gui"] = _gui;
    }

    _keyboard = std::make_shared<Keyboard>(this);
    _mouse = std::make_shared<Mouse>(this);
    _dragndrop = std::make_shared<DragNDrop>(this);

    if (_keyboard)
    {
        _keyboard->setName("keyboard");
        _objects[_keyboard->getName()] = _keyboard;
    }
    if (_mouse)
    {
        _mouse->setName("mouse");
        _objects["mouse"] = _mouse;
    }
    if (_dragndrop)
    {
        _dragndrop->setName("dragndrop");
        _objects[_dragndrop->getName()] = _dragndrop;
    }

#if HAVE_GPHOTO and HAVE_OPENCV
    // Initialize the color calibration object
    _colorCalibrator = std::make_shared<ColorCalibrator>(this);
    _colorCalibrator->setName("colorCalibrator");
    _objects["colorCalibrator"] = _colorCalibrator;
#endif

#if HAVE_CALIMIRO
    _geometricCalibrator = std::make_shared<GeometricCalibrator>(this);
    _geometricCalibrator->setName("geometricCalibrator");
    _objects["geometricCalibrator"] = _geometricCalibrator;

    _texCoordGenerator = std::make_shared<TexCoordGenerator>(this);
    _texCoordGenerator->setName("texCoordGenerator");
    _objects["texCoordGenerator"] = _texCoordGenerator;
#endif
}

/*************/
void Scene::sendMessageToWorld(const std::string& message, const Values& value)
{
    RootObject::sendMessage("world", message, value);
}

/*************/
const gfx::Renderer::RendererMsgCallbackData* Scene::getRendererMsgCallbackDataPtr()
{
    _rendererMsgCallbackData.name = _name;
    _rendererMsgCallbackData.type = "Scene";
    return &_rendererMsgCallbackData;
}

/*************/
Values Scene::sendMessageToWorldWithAnswer(const std::string& message, const Values& value, const unsigned long long timeout)
{
    return sendMessageWithAnswer("world", message, value, timeout);
}

/*************/
void Scene::init(const std::string& name)
{
    _renderer = gfx::Renderer::create(_context.renderingApi);

    if (!_renderer)
        return;

    _objectLibrary = std::make_unique<ObjectLibrary>(this);
    // *Note*: strange non-elucidated issues arise if the object library
    // is not populated _before_ preloading objects, to the GUI in particular.
    // Namely, one observed issue is that destroyed a Window will make
    // the GUI disappear completely.
    // A possibility is that some OpenGL state gets initialized correctly
    // in the process, which is not otherwise. It might also be outside of
    // OpenGL. Anyway, beware when moving this away from here.
    addTask([this]() {
        const auto datapath = std::string(DATADIR);
        const std::map<std::string, std::string> files{
            {"3d_marker", datapath + "/3d_marker.obj"}, {"2d_marker", datapath + "/2d_marker.obj"}, {"camera", datapath + "/camera.obj"}, {"probe", datapath + "/probe.obj"}};

        for (const auto& file : files)
        {
            if (!_objectLibrary->loadModel(file.first, file.second))
                continue;

            auto object = _objectLibrary->getModel(file.first);
            assert(object != nullptr);
            object->setAttribute("fill", {"color"});
        }
    });

    _isInitialized = true;
    _renderer->init(name);
}

/*************/
unsigned long long Scene::updateTargetFrameDuration()
{
    auto mon = glfwGetPrimaryMonitor();
    if (!mon)
        return 0ull;

    auto vidMode = glfwGetVideoMode(mon);
    if (!vidMode)
        return 0ull;

    auto refreshRate = vidMode->refreshRate;
    if (!refreshRate)
        return 0ull;

    return static_cast<unsigned long long>(1e6 / refreshRate);
}

/*************/
void Scene::registerAttributes()
{
    addAttribute("addObject",
        [&](const Values& args) {
            addTask([=, this]() {
                std::string type = args[0].as<std::string>();
                std::string name = args[1].as<std::string>();
                std::string sceneName = args.size() > 2 ? args[2].as<std::string>() : "";

                if (sceneName == _name)
                    addObject(type, name);
                else if (_isMaster)
                    addGhost(type, name);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("addObject", "Add an object of the given name, type, and optionally the target scene");

    addAttribute("checkSceneRunning",
        [&](const Values&) {
            sendMessageToWorld("sceneLaunched", {});
            return true;
        },
        {});
    setAttributeDescription("checkSceneLaunched", "Asks the scene to notify the World that it is running");

    addAttribute("deleteObject",
        [&](const Values& args) {
            addTask([=, this]() -> void {
                // We wait until we can indeed delete the object
                bool expectedAtomicValue = false;
                while (!_objectsCurrentlyUpdated.compare_exchange_strong(expectedAtomicValue, true, std::memory_order_acquire))
                    std::this_thread::sleep_for(chrono::milliseconds(1));
                OnScopeExit
                {
                    _objectsCurrentlyUpdated.store(false, std::memory_order_release);
                };

                std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

                auto objectName = args[0].as<std::string>();
                auto object = getObject(objectName);
                if (!object)
                    return;

                for (auto& localObject : _objects)
                {
                    unlink(object, localObject.second);
                    unlink(localObject.second, object);
                }
                _objects.erase(objectName);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("deleteObject", "Delete an object given its name");

    addAttribute("duration",
        [&](const Values& args) {
            Timer::get().setDuration(args[0].as<std::string>(), args[1].as<int>());
            return true;
        },
        {'s', 'i'});
    setAttributeDescription("duration", "Set the duration of the given timer");

    addAttribute("masterClock",
        [&](const Values& args) {
            Timer::Point clock;
            clock.years = args[0].as<uint32_t>();
            clock.months = args[1].as<uint32_t>();
            clock.days = args[2].as<uint32_t>();
            clock.hours = args[3].as<uint32_t>();
            clock.mins = args[4].as<uint32_t>();
            clock.secs = args[5].as<uint32_t>();
            clock.frame = args[6].as<uint32_t>();
            clock.paused = args[7].as<bool>();
            Timer::get().setMasterClock(clock);
            return true;
        },
        {'i', 'i', 'i', 'i', 'i', 'i', 'i'});
    setAttributeDescription("masterClock", "Set the timing of the master clock");

    addAttribute("link",
        [&](const Values& args) {
            addTask([=, this]() {
                std::string src = args[0].as<std::string>();
                std::string dst = args[1].as<std::string>();
                link(src, dst);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("link", "Link the two given objects");

    addAttribute("log",
        [&](const Values& args) {
            Log::get().setLog(args[0].as<uint64_t>(), args[1].as<std::string>(), (Log::Priority)args[2].as<int>());
            return true;
        },
        {'i', 's', 'i'});
    setAttributeDescription("log", "Add an entry to the logs, given its message and priority");

    addAttribute("logToFile",
        [&](const Values& args) {
            Log::get().logToFile(args[0].as<bool>());
            return true;
        },
        {'b'});
    setAttributeDescription("logToFile", "If true, the process holding the Scene will try to write log to file");

    addAttribute("ping",
        [&](const Values&) {
            signalBufferObjectUpdated();
            sendMessageToWorld("pong", {_name});
            return true;
        },
        {});
    setAttributeDescription("ping", "Ping the World");

    addAttribute("sync",
        [&](const Values&) {
            addTask([this]() { sendMessageToWorld("answerMessage", {"sync", _name}); });
            return true;
        },
        {});
    setAttributeDescription("sync", "Dummy message to make sure all previous messages have been processed by the Scene.");

    addAttribute("remove",
        [&](const Values& args) {
            addTask([=, this]() {
                std::string name = args[1].as<std::string>();
                remove(name);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("remove", "Remove the object of the given name");

    addAttribute("setMaster",
        [&](const Values& args) {
            addTask([=, this]() {
                if (args.empty())
                    setAsMaster();
                else
                    setAsMaster(args[0].as<std::string>());
            });
            return true;
        },
        {});
    setAttributeDescription("setMaster", "Set this Scene as master, can give the configuration file path as a parameter");

    addAttribute("start",
        [&](const Values&) {
            _started = true;
            sendMessageToWorld("answerMessage", {"start", _name});
            return true;
        },
        {});
    setAttributeDescription("start", "Start the Scene main loop");

    addAttribute("stop",
        [&](const Values&) {
            _started = false;
            return true;
        },
        {});
    setAttributeDescription("stop", "Stop the Scene main loop");

    addAttribute("swapTest",
        [&](const Values& args) {
            addTask([=, this]() {
                std::lock_guard<std::recursive_mutex> lock(_objectsMutex);
                for (auto& obj : _objects)
                    if (obj.second->getType() == "window")
                        std::dynamic_pointer_cast<Window>(obj.second)->setAttribute("swapTest", {args[0].as<bool>()});
            });
            return true;
        },
        {'i'});
    setAttributeDescription("swapTest", "Activate video swap test if set to anything but 0");

    addAttribute("swapTestColor",
        [&](const Values& args) {
            addTask([=, this]() {
                std::lock_guard<std::recursive_mutex> lock(_objectsMutex);
                for (auto& obj : _objects)
                    if (obj.second->getType() == "window")
                        std::dynamic_pointer_cast<Window>(obj.second)->setAttribute("swapTestColor", args);
            });
            return true;
        },
        {});
    setAttributeDescription("swapTestColor", "Set the swap test color");

    addAttribute("syncScenes",
        [&](const Values& /*args*/) {
            _doUploadTextures = true;
            _lastSyncMessageDate = Timer::getTime();
            return true;
        },
        {});
    setAttributeDescription("uploadTextures", "Signal that textures should be uploaded right away");

    addAttribute("quit",
        [&](const Values&) {
            addTask([this]() {
                _started = false;
                _isRunning = false;
            });
            return true;
        },
        {});
    setAttributeDescription("quit", "Ask the Scene to quit");

    addAttribute("unlink",
        [&](const Values& args) {
            addTask([=, this]() {
                std::string src = args[0].as<std::string>();
                std::string dst = args[1].as<std::string>();
                unlink(src, dst);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("unlink", "Unlink the two given objects");

    addAttribute("wireframe",
        [&](const Values& args) {
            addTask([=, this]() {
                std::lock_guard<std::recursive_mutex> lock(_objectsMutex);
                for (auto& obj : _objects)
                    if (obj.second->getType() == "camera")
                        std::dynamic_pointer_cast<Camera>(obj.second)->setAttribute("wireframe", args);
            });

            return true;
        },
        {'b'});
    setAttributeDescription("wireframe", "Show all meshes as wireframes if true");

#if HAVE_GPHOTO and HAVE_OPENCV
    addAttribute("calibrateColor",
        [&](const Values&) {
            auto calibrator = std::dynamic_pointer_cast<ColorCalibrator>(_colorCalibrator);
            if (calibrator)
                calibrator->update();
            return true;
        },
        {});
    setAttributeDescription("calibrateColor", "Launch projectors color calibration");

    addAttribute("calibrateColorResponseFunction",
        [&](const Values&) {
            auto calibrator = std::dynamic_pointer_cast<ColorCalibrator>(_colorCalibrator);
            if (calibrator)
                calibrator->updateCRF();
            return true;
        },
        {});
    setAttributeDescription("calibrateColorResponseFunction", "Launch the camera color calibration");
#endif

#if HAVE_CALIMIRO
    addAttribute("calibrateGeometry",
        [&](const Values&) {
            auto calibrator = std::dynamic_pointer_cast<GeometricCalibrator>(_geometricCalibrator);
            if (calibrator)
                calibrator->calibrate();
            return true;
        },
        {});
#endif

    addAttribute("runInBackground",
        [&](const Values& args) {
            _runInBackground = args[0].as<bool>();
            return true;
        },
        {'b'});
    setAttributeDescription("runInBackground", "If true, Splash will run in the background (useful for background processing)");

    addAttribute("swapInterval",
        [&](const Values& args) {
            _swapInterval = std::max(-1, args[0].as<int>());
            _targetFrameDuration = updateTargetFrameDuration();
            return true;
        },
        [&]() -> Values { return {(int)_swapInterval}; },
        {'i'});
    setAttributeDescription("swapInterval", "Set the interval between two video frames. 1 is synced, 0 is not, -1 to sync when possible");
}

/*************/
void Scene::initializeTree()
{
    _tree.addCallbackToLeafAt("/world/attributes/masterClock",
        [](const Value& value, const chrono::system_clock::time_point& /*timestamp*/) {
            auto args = value.as<Values>();
            Timer::Point clock;
            clock.years = args[0].as<uint32_t>();
            clock.months = args[1].as<uint32_t>();
            clock.days = args[2].as<uint32_t>();
            clock.hours = args[3].as<uint32_t>();
            clock.mins = args[4].as<uint32_t>();
            clock.secs = args[5].as<uint32_t>();
            clock.frame = args[6].as<uint32_t>();
            clock.paused = args[7].as<bool>();
            Timer::get().setMasterClock(clock);
        },
        true);

    _tree.setName(_name);
    _tree.createBranchAt("/" + _name);
    _tree.createBranchAt("/" + _name + "/attributes");
    _tree.createBranchAt("/" + _name + "/commands");
    _tree.createBranchAt("/" + _name + "/durations");
    _tree.createBranchAt("/" + _name + "/logs");
    _tree.createBranchAt("/" + _name + "/objects");
}

} // namespace Splash
