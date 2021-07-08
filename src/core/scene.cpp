#include "./core/scene.h"

#include <list>
#include <utility>

#include <Tracy.hpp>
#include <TracyOpenGL.hpp>

#include "./controller/controller_blender.h"
#include "./controller/controller_gui.h"
#include "./core/constants.h"
#include "./core/link.h"
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

// clang-format off
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#include <GL/glxext.h>
// clang-format on

#if HAVE_CALIMIRO
#include "./controller/geometriccalibrator.h"
#include "./controller/texcoordgenerator.h"
#endif

namespace chrono = std::chrono;

namespace Splash
{

bool Scene::_hasNVSwapGroup{false};
std::vector<int> Scene::_glVersion{0, 0};
std::string Scene::_glVendor{};
std::string Scene::_glRenderer{};
std::vector<std::string> Scene::_ghostableTypes{"camera", "warp"};

/*************/
Scene::Scene(Context context)
    : RootObject(context)
    , _objectLibrary(dynamic_cast<RootObject*>(this))
{
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
}

/*************/
Scene::~Scene()
{
    // Cleanup every object
    if (_mainWindow)
    {
        _mainWindow->setAsCurrentContext();
        std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex); // We don't want any friend to try accessing the objects

        // Free objects cleanly
        for (auto& obj : _objects)
            obj.second.reset();
        _objects.clear();

        _mainWindow->releaseContext();
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
            else if (obj->getType() == "window" && !_guiLinkedToWindow)
            {
                link(std::dynamic_pointer_cast<GraphObject>(_gui), obj);
                _guiLinkedToWindow = true;
            }
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
        TracyGpuZone("Upload textures");
        ZoneScopedN("Upload textures");

        Timer::get() << "textureUpload";
        std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);
        for (auto& obj : _objects)
        {
            auto texture = std::dynamic_pointer_cast<Texture>(obj.second);
            if (texture)
            {
                TracyGpuZone("Uploading a texture");
                ZoneScopedN("Uploading a texture");
                ZoneName(obj.first.c_str(), obj.first.size());

                texture->update();
            }
        }
        Timer::get() >> "textureUpload";
    }

    {
        TracyGpuZone("Scene rendering");
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
            TracyGpuZone("Render bin");
            ZoneScopedN("Render bin");

            if (objPriority.second.size() != 0)
            {
                const auto type = objPriority.second[0]->getType();
                Timer::get() << type;
                ZoneName(type.c_str(), type.size());
            }

            for (auto& obj : objPriority.second)
            {
                TracyGpuZone("Rendering an object");
                ZoneScopedN("Rendering an object");
                ZoneName(obj->getName().c_str(), obj->getName().size());

                obj->update();

                auto objectCategory = obj->getCategory();
                if (objectCategory == GraphObject::Category::MESH)
                    if (obj->wasUpdated())
                    {
                        // If a mesh has been updated, force blending update
                        addTask([=]() { std::dynamic_pointer_cast<Blender>(_blender)->forceUpdate(); });
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
        TracyGpuZone("Swap");
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
#ifndef PROFILE_GPU
    ProfilerGL::get().clearTimings();
#endif
}

/*************/
void Scene::run()
{
    tracy::SetThreadName("Scene");

    if (!_mainWindow)
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - No rendering context has been created" << Log::endl;
        return;
    }

    _mainWindow->setAsCurrentContext();
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

        FrameMarkEnd("Scene");
    }
    _mainWindow->releaseContext();

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

    _gui = std::make_shared<Gui>(_mainWindow, this);
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
Values Scene::sendMessageToWorldWithAnswer(const std::string& message, const Values& value, const unsigned long long timeout)
{
    return sendMessageWithAnswer("world", message, value, timeout);
}

/*************/
std::shared_ptr<GlWindow> Scene::getNewSharedWindow(const std::string& name)
{
    std::string windowName;
    name.size() == 0 ? windowName = "Splash::Window" : windowName = "Splash::" + name;

    if (!_mainWindow)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Main window does not exist, unable to create new shared window" << Log::endl;
        return {nullptr};
    }

    // The GL version is the same as in the initialization, so we don't have to reset it here
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(512, 512, windowName.c_str(), NULL, _mainWindow->get());
    if (!window)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
        return {nullptr};
    }
    auto glWindow = std::make_shared<GlWindow>(window, _mainWindow->get());

    glWindow->setAsCurrentContext();
#ifdef DEBUGGL
    glDebugMessageCallback(Scene::glMsgCallback, (void*)this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif

#ifdef GLX_NV_swap_group
    if (_maxSwapGroups)
    {
        PFNGLXJOINSWAPGROUPNVPROC nvGLJoinSwapGroup = (PFNGLXJOINSWAPGROUPNVPROC)glfwGetProcAddress("glXJoinSwapGroupNV");
        auto nvResult = nvGLJoinSwapGroup(glfwGetX11Display(), glfwGetGLXWindow(window), 1);
        if (nvResult)
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " successfully joined the NV swap group" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " couldn't join the NV swap group" << Log::endl;
    }

    if (_maxSwapBarriers)
    {
        PFNGLXBINDSWAPBARRIERNVPROC nvGLBindSwapBarrier = (PFNGLXBINDSWAPBARRIERNVPROC)glfwGetProcAddress("glXBindSwapBarrierNV");
        auto nvResult = nvGLBindSwapBarrier(glfwGetX11Display(), 1, 1);
        if (nvResult)
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " successfully bind the NV swap barrier" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " couldn't bind the NV swap barrier" << Log::endl;
    }
#endif
    glWindow->releaseContext();

    return glWindow;
}

/*************/
std::vector<int> Scene::findGLVersion()
{
    std::vector<std::vector<int>> glVersionList{{4, 5}};
    std::vector<int> detectedVersion{0, 0};

    for (auto version : glVersionList)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version[0]);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version[1]);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
        glfwWindowHint(GLFW_VISIBLE, false);
        GLFWwindow* window = glfwCreateWindow(512, 512, "test_window", NULL, NULL);

        if (window)
        {
            detectedVersion = version;
            glfwDestroyWindow(window);
            break;
        }
    }

    return detectedVersion;
}

/*************/
void Scene::init(const std::string& name)
{
    glfwSetErrorCallback(Scene::glfwErrorCallback);

    // GLFW stuff
    if (!glfwInit())
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
        _isInitialized = false;
        return;
    }

    auto glVersion = findGLVersion();
    if (glVersion[0] == 0)
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to find a suitable GL version (higher than 4.3)" << Log::endl;
        _isInitialized = false;
        return;
    }

    _glVersion = glVersion;
    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL version: " << glVersion[0] << "." << glVersion[1] << Log::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glVersion[0]);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glVersion[1]);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef DEBUGGL
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
#endif
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_VISIBLE, false);

    GLFWwindow* window = glfwCreateWindow(512, 512, name.c_str(), NULL, NULL);

    if (!window)
    {
        Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        _isInitialized = false;
        return;
    }

    _mainWindow = std::make_shared<GlWindow>(window, window);
    _isInitialized = true;

    _mainWindow->setAsCurrentContext();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Get hardware information
    _glVendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    _glRenderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL vendor: " << _glVendor << Log::endl;
    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL renderer: " << _glRenderer << Log::endl;

// Activate GL debug messages
#ifdef DEBUGGL
    glDebugMessageCallback(Scene::glMsgCallback, (void*)this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif

// Check for swap groups
#ifdef GLX_NV_swap_group
    if (glfwExtensionSupported("GLX_NV_swap_group"))
    {
        PFNGLXQUERYMAXSWAPGROUPSNVPROC nvGLQueryMaxSwapGroups = (PFNGLXQUERYMAXSWAPGROUPSNVPROC)glfwGetProcAddress("glXQueryMaxSwapGroupsNV");
        if (!nvGLQueryMaxSwapGroups(glfwGetX11Display(), 0, &_maxSwapGroups, &_maxSwapBarriers))
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Unable to get NV max swap groups / barriers" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - NV max swap groups: " << _maxSwapGroups << " / barriers: " << _maxSwapBarriers << Log::endl;

        if (_maxSwapGroups != 0)
            _hasNVSwapGroup = true;
    }
#endif
    _mainWindow->releaseContext();

    // Create the link and connect to the World
    _link = std::make_unique<Link>(this, name);
    _link->connectTo("world");
    sendMessageToWorld("sceneLaunched", {});
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
void Scene::glfwErrorCallback(int /*code*/, const char* msg)
{
    Log::get() << Log::WARNING << "Scene::glfwErrorCallback - " << msg << Log::endl;
}

/*************/
void Scene::glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, void* /*userParam*/)
{
    std::string typeString{"OTHER"};
    std::string severityString{""};
    Log::Priority logType{Log::MESSAGE};

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        typeString = "Error";
        logType = Log::ERROR;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeString = "Deprecated behavior";
        logType = Log::WARNING;
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeString = "Undefined behavior";
        logType = Log::ERROR;
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeString = "Portability";
        logType = Log::WARNING;
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeString = "Performance";
        logType = Log::WARNING;
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeString = "Other";
        logType = Log::MESSAGE;
        break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_LOW:
        severityString = "low";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityString = "medium";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityString = "high";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        // Disable notifications, they are far too verbose
        return;
        // severityString = "notification";
        // break;
    }

    Log::get() << logType << "GL::debug - [" << typeString << "::" << severityString << "] - " << message << Log::endl;
}

/*************/
void Scene::registerAttributes()
{
    addAttribute("addObject",
        [&](const Values& args) {
            addTask([=]() {
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

    addAttribute("deleteObject",
        [&](const Values& args) {
            addTask([=]() -> void {
                // We wait until we can indeed delete the object
                bool expectedAtomicValue = false;
                while (!_objectsCurrentlyUpdated.compare_exchange_strong(expectedAtomicValue, true, std::memory_order_acquire))
                    std::this_thread::sleep_for(chrono::milliseconds(1));
                OnScopeExit { _objectsCurrentlyUpdated.store(false, std::memory_order_release); };

                std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

                auto objectName = args[0].as<std::string>();
                auto object = getObject(objectName);
                if (!object)
                    return;

                for (auto& localObject : _objects)
                    unlink(object, localObject.second);
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
            addTask([=]() {
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
            addTask([=]() { sendMessageToWorld("answerMessage", {"sync", _name}); });
            return true;
        },
        {});
    setAttributeDescription("sync", "Dummy message to make sure all previous messages have been processed by the Scene.");

    addAttribute("remove",
        [&](const Values& args) {
            addTask([=]() {
                std::string name = args[1].as<std::string>();
                remove(name);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("remove", "Remove the object of the given name");

    addAttribute("setMaster",
        [&](const Values& args) {
            addTask([=]() {
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
            addTask([=]() {
                std::lock_guard<std::recursive_mutex> lock(_objectsMutex);
                for (auto& obj : _objects)
                    if (obj.second->getType() == "window")
                        std::dynamic_pointer_cast<Window>(obj.second)->setAttribute("swapTest", args);
            });
            return true;
        },
        {'i'});
    setAttributeDescription("swapTest", "Activate video swap test if set to anything but 0");

    addAttribute("swapTestColor",
        [&](const Values& args) {
            addTask([=]() {
                std::lock_guard<std::recursive_mutex> lock(_objectsMutex);
                for (auto& obj : _objects)
                    if (obj.second->getType() == "window")
                        std::dynamic_pointer_cast<Window>(obj.second)->setAttribute("swapTestColor", args);
            });
            return true;
        },
        {});
    setAttributeDescription("swapTestColor", "Set the swap test color");

    addAttribute("uploadTextures",
        [&](const Values& /*args*/) {
            _doUploadTextures = true;
            return true;
        },
        {});
    setAttributeDescription("uploadTextures", "Signal that textures should be uploaded right away");

    addAttribute("quit",
        [&](const Values&) {
            addTask([=]() {
                _started = false;
                _isRunning = false;
            });
            return true;
        },
        {});
    setAttributeDescription("quit", "Ask the Scene to quit");

    addAttribute("unlink",
        [&](const Values& args) {
            addTask([=]() {
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
            addTask([=]() {
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
