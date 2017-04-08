#include "scene.h"

#include <utility>

#include "./camera.h"
#include "./controller_blender.h"
#include "./controller_gui.h"
#include "./filter.h"
#include "./geometry.h"
#include "./image.h"
#include "./link.h"
#include "./log.h"
#include "./mesh.h"
#include "./object.h"
#include "./osUtils.h"
#include "./queue.h"
#include "./texture.h"
#include "./texture_image.h"
#include "./threadpool.h"
#include "./timer.h"
#include "./userInput_dragndrop.h"
#include "./userInput_joystick.h"
#include "./userInput_keyboard.h"
#include "./userInput_mouse.h"
#include "./warp.h"
#include "./window.h"

#if HAVE_GPHOTO
#include "./colorcalibrator.h"
#endif

#if HAVE_OSX
#include "./texture_syphon.h"
#else
// clang-format off
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#include <GL/glxext.h>
// clang-format on
#endif

using namespace std;

namespace Splash
{

bool Scene::_hasNVSwapGroup{false};
vector<int> Scene::_glVersion{0, 0};

/*************/
std::vector<int> Scene::getGLVersion()
{
    return _glVersion;
}

/*************/
bool Scene::getHasNVSwapGroup()
{
    return _hasNVSwapGroup;
}

/*************/
Scene::Scene(const std::string& name)
{
    Log::get() << Log::DEBUGGING << "Scene::Scene - Scene created successfully" << Log::endl;

    _type = "scene";
    _isRunning = true;
    _name = name;

    _blender = make_shared<Blender>(this);
    if (_blender)
    {
        _blender->setName("blender");
        _objects["blender"] = _blender;
    }

    registerAttributes();

    init(_name);
}

/*************/
Scene::~Scene()
{
    unique_lock<mutex> lockTexture(_textureUploadMutex);
    _textureUploadCondition.notify_all();
    lockTexture.unlock();
    _textureUploadFuture.get();

    // Cleanup every object
    _mainWindow->setAsCurrentContext();
    lock_guard<recursive_mutex> lockObjects(_objectsMutex); // We don't want any friend to try accessing the objects

    // Free objects cleanly
    for (auto& obj : _objects)
        obj.second.reset();
    _objects.clear();
    for (auto& obj : _objects)
        obj.second.reset();
    _ghostObjects.clear();

    _mainWindow->releaseContext();

    Log::get() << Log::DEBUGGING << "Scene::~Scene - Destructor" << Log::endl;
}

/*************/
std::shared_ptr<BaseObject> Scene::add(const string& type, const string& name)
{
    Log::get() << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - Creating object of type " << type << Log::endl;

    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    // If we run in background mode, don't create any window
    if (_runInBackground && type == "window")
        return {};

    // Check whether an object of this name already exists
    auto objectIt = _objects.find(name);
    if (objectIt != _objects.end())
        return {};

    // Create the wanted object
    if (!_mainWindow->setAsCurrentContext())
        Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;

    auto obj = _factory->create(type);
    _mainWindow->releaseContext();

    // Add the object to the objects list
    if (obj.get() != nullptr)
    {
        obj->setRemoteType(type); // Not all objects have remote types, but this doesn't harm

        obj->setId(getId());
        auto realName = obj->setName(name);
        if (realName == string())
            _objects[to_string(obj->getId())] = obj;
        else
            _objects[realName] = obj;

        // Some objects have to be connected to the gui (if the Scene is master)
        if (_gui != nullptr)
        {
            if (obj->getType() == "object")
                link(obj, dynamic_pointer_cast<BaseObject>(_gui));
            else if (obj->getType() == "window" && !_guiLinkedToWindow)
            {
                link(dynamic_pointer_cast<BaseObject>(_gui), obj);
                _guiLinkedToWindow = true;
            }
        }

        // Special treatment for the windows
        if (type == "window")
            obj->setAttribute("swapInterval", {_swapInterval});
    }

    return obj;
}

/*************/
void Scene::addGhost(const string& type, const string& name)
{
    // Currently, only Cameras can be ghosts
    if (type != string("camera") && type != string("warp"))
        return;

    // Check whether an object of this name already exists
    auto objectIt = _ghostObjects.find(name);
    if (objectIt != _ghostObjects.end())
        return;

    Log::get() << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - Creating ghost object of type " << type << Log::endl;

    // Add the object for real ...
    std::shared_ptr<BaseObject> obj = add(type, name);
    if (obj)
    {
        // And move it to _ghostObjects
        lock_guard<recursive_mutex> lockObjects(_objectsMutex);
        _objects.erase(obj->getName());
        _ghostObjects[obj->getName()] = obj;
    }
}

/*************/
Values Scene::getAttributeFromObject(const string& name, const string& attribute)
{
    lock_guard<recursive_mutex> lockObjects(_objectsMutex);
    auto objectIt = _objects.find(name);

    Values values;
    if (objectIt != _objects.end())
    {
        auto& object = objectIt->second;
        object->getAttribute(attribute, values);
    }
    // Ask the World if it knows more about this object
    else
    {
        auto answer = sendMessageToWorldWithAnswer("getAttribute", {name, attribute});
        for (unsigned int i = 1; i < answer.size(); ++i)
            values.push_back(answer[i]);
    }

    return values;
}

/*************/
Values Scene::getAttributeDescriptionFromObject(const string& name, const string& attribute)
{
    lock_guard<recursive_mutex> lockObjects(_objectsMutex);
    auto objectIt = _objects.find(name);

    Values values;
    if (objectIt != _objects.end())
    {
        auto& object = objectIt->second;
        values.push_back(object->getAttributeDescription(attribute));
    }

    // Ask the World if it knows more about this object
    if (values.size() == 0 || values[0].as<string>() == "")
    {
        auto answer = sendMessageToWorldWithAnswer("getAttributeDescription", {name, attribute}, 10000);
        if (answer.size() != 0)
        {
            values.clear();
            values.push_back(answer[1]);
        }
    }

    return values;
}

/*************/
Json::Value Scene::getConfigurationAsJson()
{
    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    Json::Value root;

    root[_name] = BaseObject::getConfigurationAsJson();
    // Save objects attributes
    for (auto& obj : _objects)
        if (obj.second->getSavable())
            root[obj.first] = obj.second->getConfigurationAsJson();

    // Save links
    Values links;
    for (auto& obj : _objects)
    {
        if (!obj.second->getSavable())
            continue;

        auto linkedObjects = obj.second->getLinkedObjects();
        for (auto& linkedObj : linkedObjects)
        {
            if (!linkedObj->getSavable())
                continue;

            links.push_back(Values({linkedObj->getName(), obj.second->getName()}));
        }
    }

    root["links"] = getValuesAsJson(links);

    return root;
}

/*************/
bool Scene::link(const string& first, const string& second)
{
    std::shared_ptr<BaseObject> source(nullptr);
    std::shared_ptr<BaseObject> sink(nullptr);

    lock_guard<recursive_mutex> lockObjects(_objectsMutex);
    if (_objects.find(first) != _objects.end())
        source = _objects[first];
    if (_objects.find(second) != _objects.end())
        sink = _objects[second];

    if (source.get() != nullptr && sink.get() != nullptr)
        return link(source, sink);
    else
        return false;
}

/*************/
bool Scene::link(const std::shared_ptr<BaseObject>& first, const std::shared_ptr<BaseObject>& second)
{
    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    glfwMakeContextCurrent(_mainWindow->get());
    bool result = second->linkTo(first);
    glfwMakeContextCurrent(NULL);

    return result;
}

/*************/
void Scene::unlink(const string& first, const string& second)
{
    std::shared_ptr<BaseObject> source(nullptr);
    std::shared_ptr<BaseObject> sink(nullptr);

    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    if (_objects.find(first) != _objects.end())
        source = _objects[first];
    if (_objects.find(second) != _objects.end())
        sink = _objects[second];

    if (source.get() != nullptr && sink.get() != nullptr)
        unlink(source, sink);
}

/*************/
void Scene::unlink(const std::shared_ptr<BaseObject>& first, const std::shared_ptr<BaseObject>& second)
{
    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    glfwMakeContextCurrent(_mainWindow->get());
    second->unlinkFrom(first);
    glfwMakeContextCurrent(NULL);
}

/*************/
bool Scene::linkGhost(const string& first, const string& second)
{
    std::shared_ptr<BaseObject> source(nullptr);
    std::shared_ptr<BaseObject> sink(nullptr);

    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    if (_ghostObjects.find(first) != _ghostObjects.end())
        source = _ghostObjects[first];
    else if (_objects.find(first) != _objects.end())
        source = _objects[first];
    else
        return false;

    if (_ghostObjects.find(second) != _ghostObjects.end())
        sink = _ghostObjects[second];
    else if (_objects.find(second) != _objects.end())
        sink = _objects[second];
    else
        return false;

    return link(source, sink);
}

/*************/
void Scene::unlinkGhost(const string& first, const string& second)
{
    std::shared_ptr<BaseObject> source(nullptr);
    std::shared_ptr<BaseObject> sink(nullptr);

    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    if (_ghostObjects.find(first) != _ghostObjects.end())
        source = _ghostObjects[first];
    else if (_objects.find(first) != _objects.end())
        source = _objects[first];
    else
        return;

    if (_ghostObjects.find(second) != _ghostObjects.end())
        sink = _ghostObjects[second];
    else if (_objects.find(second) != _objects.end())
        sink = _objects[second];
    else
        return;

    unlink(source, sink);
}

/*************/
void Scene::remove(const string& name)
{
    std::shared_ptr<BaseObject> obj;

    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    if (_objects.find(name) != _objects.end())
        _objects.erase(name);
    else if (_ghostObjects.find(name) != _ghostObjects.end())
        _ghostObjects.erase(name);
}

/*************/
void Scene::render()
{
    // Create lists of objects to update and to render
    map<Priority, vector<shared_ptr<BaseObject>>> objectList{};
    {
        lock_guard<recursive_mutex> lockObjects(_objectsMutex);
        for (auto& obj : _objects)
        {
            auto priority = obj.second->getRenderingPriority();
            if (priority == Priority::NO_RENDER)
                continue;

            auto listIt = objectList.find(priority);
            if (listIt == objectList.end())
            {
                auto entry = objectList.emplace(std::make_pair(priority, vector<shared_ptr<BaseObject>>()));
                if (entry.second == true)
                    listIt = entry.first;
                else
                    continue;
            }

            listIt->second.push_back(obj.second);
        }
    }

    // Update and render the objects
    // See BaseObject::getRenderingPriority() for precision about priorities
    bool firstTextureSync = true; // Sync with the texture upload the first time we need textures
    bool firstWindowSync = true;  // Sync with the texture upload the last time we need textures
    auto textureLock = unique_lock<Spinlock>(_textureMutex, defer_lock);
    for (auto& objPriority : objectList)
    {
        // If the objects needs some Textures, we need to sync
        if (firstTextureSync && objPriority.first > Priority::BLENDING && objPriority.first < Priority::POST_CAMERA)
        {
            // We wait for textures to be uploaded, and we prevent any upload while rendering
            // cameras to prevent tearing
            textureLock.lock();
            glWaitSync(_textureUploadFence, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync(_textureUploadFence);
            firstTextureSync = false;
        }

        if (objPriority.second.size() != 0)
            Timer::get() << objPriority.second[0]->getType();

        for (auto& obj : objPriority.second)
        {
            obj->update();

            if (obj->getCategory() == BaseObject::Category::MESH)
                if (obj->wasUpdated())
                {
                    // If a mesh has been updated, force blending update
                    dynamic_pointer_cast<Blender>(_blender)->forceUpdate();
                    obj->setNotUpdated();
                }

            if (obj->getCategory() == BaseObject::Category::IMAGE)
                if (obj->wasUpdated())
                    obj->setNotUpdated();

            obj->render();
        }

        if (objPriority.second.size() != 0)
            Timer::get() >> objPriority.second[0]->getType();

        if (firstWindowSync && objPriority.first >= Priority::POST_CAMERA)
        {
            _cameraDrawnFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            if (textureLock.owns_lock())
                textureLock.unlock();
            firstWindowSync = false;
        }
    }

    // Swap all buffers at once
    Timer::get() << "swap";
    for (auto& obj : _objects)
        if (obj.second->getType() == "window")
            dynamic_pointer_cast<Window>(obj.second)->swapBuffers();
    Timer::get() >> "swap";
}

/*************/
void Scene::run()
{
    _textureUploadFuture = async(std::launch::async, [&]() { textureUploadRun(); });

    while (_isRunning)
    {
        // Execute waiting tasks
        runTasks();

        if (!_started)
        {
            this_thread::sleep_for(chrono::milliseconds(50));
            continue;
        }

        Timer::get() << "sceneLoop";

        {
            _mainWindow->setAsCurrentContext();
            render();
            _mainWindow->releaseContext();
        }

        Timer::get() >> "sceneLoop";

        Timer::get() << "inputsUpdate";
        updateInputs();
        Timer::get() >> "inputsUpdate";
    }
}

/*************/
void Scene::updateInputs()
{
    glfwPollEvents();

    auto keyboard = dynamic_pointer_cast<Keyboard>(_keyboard);
    if (keyboard)
        _gui->setKeyboardState(keyboard->getState(_name));

    auto mouse = dynamic_pointer_cast<Mouse>(_mouse);
    if (mouse)
        _gui->setMouseState(mouse->getState(_name));

    // Check if we should quit.
    if (Window::getQuitFlag())
        sendMessageToWorld("quit");
}

/*************/
void Scene::textureUploadRun()
{
    while (_isRunning)
    {
        if (!_started)
        {
            this_thread::sleep_for(chrono::milliseconds(50));
            continue;
        }

        unique_lock<mutex> lockUploadTexture(_textureUploadMutex);
        _textureUploadCondition.wait(lockUploadTexture);

        if (!_isRunning)
            break;

        unique_lock<Spinlock> lockTexture(_textureMutex);

        _textureUploadWindow->setAsCurrentContext();
        glFlush();
        glWaitSync(_cameraDrawnFence, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(_cameraDrawnFence);

        Timer::get() << "textureUpload";

        vector<shared_ptr<Texture>> textures;
        bool expectedAtomicValue = false;
        if (!_objectsCurrentlyUpdated.compare_exchange_strong(expectedAtomicValue, true))
        {
            for (auto& obj : _objects)
                if (obj.second->getType().find("texture") != string::npos)
                    textures.emplace_back(dynamic_pointer_cast<Texture>(obj.second));
            _objectsCurrentlyUpdated = false;
        }

        for (auto& texture : textures)
            texture->update();

        _textureUploadFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        lockTexture.unlock();

        for (auto& texture : textures)
        {
            auto texImage = dynamic_pointer_cast<Texture_Image>(texture);
            if (texImage)
                texImage->flushPbo();
        }

        _textureUploadWindow->releaseContext();
        Timer::get() >> "textureUpload";
    }
}

/*************/
void Scene::setAsMaster(const string& configFilePath)
{
    lock_guard<recursive_mutex> lockObjects(_objectsMutex);

    _isMaster = true;
    // We have to reset the factory to reflect this change
    _factory.reset(new Factory(this));

    _mainWindow->setAsCurrentContext();
    _gui = make_shared<Gui>(_mainWindow, this);
    if (_gui)
    {
        _gui->setName("gui");
        _gui->setConfigFilePath(configFilePath);
        _objects["gui"] = _gui;
    }
    _mainWindow->releaseContext();

    _keyboard = make_shared<Keyboard>(this);
    _mouse = make_shared<Mouse>(this);
    _joystick = make_shared<Joystick>(this);
    _dragndrop = make_shared<DragNDrop>(this);

    if (_keyboard)
        _objects["keyboard"] = _keyboard;
    if (_mouse)
        _objects["mouse"] = _mouse;
    if (_joystick)
        _objects["joystick"] = _joystick;
    if (_dragndrop)
        _objects["dragndrop"] = _dragndrop;

#if HAVE_GPHOTO
    // Initialize the color calibration object
    _colorCalibrator = make_shared<ColorCalibrator>(this);
    _colorCalibrator->setName("colorCalibrator");
    _objects["colorCalibrator"] = dynamic_pointer_cast<BaseObject>(_colorCalibrator);
#endif
}

/*************/
void Scene::setAsWorldScene()
{
    // First we create a single camera linked to a single window
    add("camera", "_camera");
    add("window", "_window");
    link("_camera", "_window");

    // Then we need to connect all Objects to the camera
    lock_guard<recursive_mutex> lock(_objectsMutex);
    for (auto& obj : _objects)
        link(obj.first, "_camera");
}

/*************/
void Scene::sendMessageToWorld(const string& message, const Values& value)
{
    RootObject::sendMessage("world", message, value);
}

/*************/
Values Scene::sendMessageToWorldWithAnswer(const string& message, const Values& value, const unsigned long long timeout)
{
    return sendMessageWithAnswer("world", message, value, timeout);
}

/*************/
void Scene::waitTextureUpload()
{
    lock_guard<mutex> lockTexture(_textureUploadMutex);
    glWaitSync(_textureUploadFence, 0, GL_TIMEOUT_IGNORED);
}

/*************/
shared_ptr<GlWindow> Scene::getNewSharedWindow(const string& name)
{
    string windowName;
    name.size() == 0 ? windowName = "Splash::Window" : windowName = "Splash::" + name;

    if (!_mainWindow)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Main window does not exist, unable to create new shared window" << Log::endl;
        return {nullptr};
    }

    // The GL version is the same as in the initialization, so we don't have to reset it here
    glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(512, 512, windowName.c_str(), NULL, _mainWindow->get());
    if (!window)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
        return {nullptr};
    }
    auto glWindow = make_shared<GlWindow>(window, _mainWindow->get());

    glWindow->setAsCurrentContext();
#if not HAVE_OSX
#ifdef DEBUGGL
    glDebugMessageCallback(Scene::glMsgCallback, (void*)this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif
#endif

#if not HAVE_OSX
#ifdef GLX_NV_swap_group
    if (_maxSwapGroups)
    {
        PFNGLXJOINSWAPGROUPNVPROC nvGLJoinSwapGroup = (PFNGLXJOINSWAPGROUPNVPROC)glfwGetProcAddress("glXJoinSwapGroupNV");
        PFNGLXBINDSWAPBARRIERNVPROC nvGLBindSwapBarrier = (PFNGLXBINDSWAPBARRIERNVPROC)glfwGetProcAddress("glXBindSwapBarrierNV");

        bool nvResult = true;
        nvResult = nvGLJoinSwapGroup(glfwGetX11Display(), glfwGetGLXWindow(window), 1);
        if (nvResult)
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " successfully joined the NV swap group" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " couldn't join the NV swap group" << Log::endl;

        nvResult = nvGLBindSwapBarrier(glfwGetX11Display(), 1, 1);
        if (nvResult)
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " successfully bind the NV swap barrier" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << windowName << " couldn't bind the NV swap barrier" << Log::endl;
    }
#endif
#endif
    glWindow->releaseContext();

    return glWindow;
}

/*************/
Values Scene::getObjectsNameByType(const string& type)
{
    lock_guard<recursive_mutex> lock(_objectsMutex);
    Values list;
    for (auto& obj : _objects)
        if (obj.second->getType() == type)
            list.push_back(obj.second->getName());
    for (auto& obj : _ghostObjects)
        if (obj.second->getType() == type)
            list.push_back(obj.second->getName());
    return list;
}

/*************/
vector<int> Scene::findGLVersion()
{
    vector<vector<int>> glVersionList{{4, 3}, {4, 0}};
    vector<int> detectedVersion{0, 0};

    for (auto version : glVersionList)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version[0]);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version[1]);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if HAVE_OSX
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
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
void Scene::init(const string& name)
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
    glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_VISIBLE, false);
#if HAVE_OSX
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(512, 512, name.c_str(), NULL, NULL);

    if (!window)
    {
        Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        _isInitialized = false;
        return;
    }

    _mainWindow = make_shared<GlWindow>(window, window);
    _isInitialized = true;

    _mainWindow->setAsCurrentContext();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

// Activate GL debug messages
#if not HAVE_OSX
#ifdef DEBUGGL
    glDebugMessageCallback(Scene::glMsgCallback, (void*)this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif
#endif

// Check for swap groups
#if not HAVE_OSX
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
#endif
    _mainWindow->releaseContext();

    _textureUploadWindow = getNewSharedWindow();

    // Create the link and connect to the World
    _link = make_shared<Link>(this, name);
    _link->connectTo("world");
    sendMessageToWorld("sceneLaunched", {});
}

/*************/
void Scene::glfwErrorCallback(int code, const char* msg)
{
    Log::get() << Log::WARNING << "Scene::glfwErrorCallback - " << msg << Log::endl;
}

/*************/
#ifdef HAVE_OSX
void Scene::glMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
#else
void Scene::glMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam)
#endif
{
    string typeString{"OTHER"};
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

    Log::get() << logType << "GL::debug - [" << typeString << "] - " << message << Log::endl;
}

/*************/
void Scene::registerAttributes()
{
    RootObject::registerAttributes();

    addAttribute("add",
        [&](const Values& args) {
            addTask([=]() {
                string type = args[0].as<string>();
                string name = args[1].as<string>();
                add(type, name);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("add", "Add an object of the given name and type");

    addAttribute("addGhost",
        [&](const Values& args) {
            addTask([=]() {
                string type = args[0].as<string>();
                string name = args[1].as<string>();
                addGhost(type, name);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("addGhost", "Add a ghost object of the given name and type. Only useful in the master Scene");

    addAttribute("bufferUploaded", [&](const Values& args) {
        _textureUploadCondition.notify_all();
        return true;
    });
    setAttributeDescription("bufferUploaded", "Message sent by the World to notify that new textures have been sent");

    addAttribute("config", [&](const Values& args) {
        addTask([&]() -> void {
            setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas
            Json::Value config = getConfigurationAsJson();
            string configStr = config.toStyledString();
            sendMessageToWorld("answerMessage", {"config", _name, configStr});
        });
        return true;
    });
    setAttributeDescription("config", "Ask the Scene for a JSON describing its configuration");

    addAttribute("deleteObject",
        [&](const Values& args) {
            addTask([=]() -> void {
                // We wait until we can indeed deleted the object
                bool expectedAtomicValue = false;
                while (!_objectsCurrentlyUpdated.compare_exchange_strong(expectedAtomicValue, true))
                    this_thread::sleep_for(chrono::milliseconds(1));
                OnScopeExit { _objectsCurrentlyUpdated = false; };

                lock_guard<recursive_mutex> lockObjects(_objectsMutex);

                auto objectName = args[0].as<string>();
                auto objectIt = _objects.find(objectName);
                if (objectIt == _objects.end())
                    return;

                for (auto& localObject : _objects)
                    unlink(objectIt->second, localObject.second);
                if (objectIt != _objects.end())
                    _objects.erase(objectIt);

                objectIt = _ghostObjects.find(objectName);
                if (objectIt != _ghostObjects.end())
                {
                    for (auto& ghostObject : _ghostObjects)
                        unlink(objectIt->second, ghostObject.second);
                    _ghostObjects.erase(objectIt);
                }
            });

            return true;
        },
        {'s'});
    setAttributeDescription("deleteObject", "Delete an object given its name");

    addAttribute("duration",
        [&](const Values& args) {
            Timer::get().setDuration(args[0].as<string>(), args[1].as<int>());
            return true;
        },
        {'s', 'n'});
    setAttributeDescription("duration", "Set the duration of the given timer");

    addAttribute("masterClock",
        [&](const Values& args) {
            Timer::get().setMasterClock(args);
            return true;
        },
        {'n', 'n', 'n', 'n', 'n', 'n', 'n'});
    setAttributeDescription("masterClock", "Set the timing of the master clock");

    addAttribute("flashBG",
        [&](const Values& args) {
            addTask([=]() {
                lock_guard<recursive_mutex> lockObjects(_objectsMutex);
                for (auto& obj : _objects)
                    if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
                        dynamic_pointer_cast<Camera>(obj.second)->setAttribute("flashBG", {(int)(args[0].as<int>())});
            });

            return true;
        },
        {'n'});
    setAttributeDescription("flashBG", "Switches the background color from black to light grey");

    addAttribute("getObjectsNameByType",
        [&](const Values& args) {
            addTask([=]() {
                string type = args[0].as<string>();
                Values list = getObjectsNameByType(type);
                sendMessageToWorld("answerMessage", {"getObjectsNameByType", _name, list});
            });

            return true;
        },
        {'s'});
    setAttributeDescription("getObjectsNameByType", "Get a list of the objects having the given type");

    addAttribute("link",
        [&](const Values& args) {
            addTask([=]() {
                string src = args[0].as<string>();
                string dst = args[1].as<string>();
                link(src, dst);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("link", "Link the two given objects");

    addAttribute("linkGhost",
        [&](const Values& args) {
            addTask([=]() {
                string src = args[0].as<string>();
                string dst = args[1].as<string>();
                linkGhost(src, dst);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("linkGhost", "Link the two given ghost objects");

    addAttribute("log",
        [&](const Values& args) {
            Log::get().setLog(args[0].as<string>(), (Log::Priority)args[1].as<int>());
            return true;
        },
        {'s', 'n'});
    setAttributeDescription("log", "Add an entry to the logs, given its message and priority");

    addAttribute("logToFile",
        [&](const Values& args) {
            Log::get().logToFile(args[0].as<bool>());
            return true;
        },
        {'n'});
    setAttributeDescription("logToFile", "If set to 1, the process holding the Scene will try to write log to file");

    addAttribute("ping", [&](const Values& args) {
        _textureUploadCondition.notify_all();
        sendMessageToWorld("pong", {_name});
        return true;
    });
    setAttributeDescription("ping", "Ping the World");

    addAttribute("remove",
        [&](const Values& args) {
            addTask([=]() {
                string name = args[1].as<string>();
                remove(name);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("remove", "Remove the object of the given name");

    addAttribute("renameObject",
        [&](const Values& args) {
            auto name = args[0].as<string>();
            auto newName = args[1].as<string>();

            addTask([=]() {
                lock_guard<recursive_mutex> lock(_objectsMutex);

                auto objIt = _objects.find(name);
                if (objIt != _objects.end())
                {
                    auto object = objIt->second;
                    object->setName(newName);
                    _objects[newName] = object;
                    _objects.erase(objIt);
                }
            });

            return true;
        },
        {'s', 's'});

    addAttribute("setGhost",
        [&](const Values& args) {
            addTask([=]() {
                string name = args[0].as<string>();
                string attr = args[1].as<string>();
                Values values;
                for (int i = 2; i < args.size(); ++i)
                    values.push_back(args[i]);

                if (_ghostObjects.find(name) != _ghostObjects.end())
                    _ghostObjects[name]->setAttribute(attr, values);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("setGhost", "Set a given object the given attribute");

    addAttribute("setMaster", [&](const Values& args) {
        if (args.size() == 0)
            setAsMaster();
        else
            setAsMaster(args[0].as<string>());
        return true;
    });
    setAttributeDescription("setMaster", "Set this Scene as master, can give the configuration file path as a parameter");

    addAttribute("start", [&](const Values& args) {
        _started = true;
        sendMessageToWorld("answerMessage", {"start", _name});
        return true;
    });
    setAttributeDescription("start", "Start the Scene main loop");

    addAttribute("stop", [&](const Values& args) {
        _started = false;
        return true;
    });
    setAttributeDescription("stop", "Stop the Scene main loop");

    addAttribute("swapInterval",
        [&](const Values& args) {
            _swapInterval = max(-1, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {(int)_swapInterval}; },
        {'n'});
    setAttributeDescription("swapInterval", "Set the interval between two video frames. 1 is synced, 0 is not");

    addAttribute("swapTest", [&](const Values& args) {
        addTask([=]() {
            lock_guard<recursive_mutex> lock(_objectsMutex);
            for (auto& obj : _objects)
                if (obj.second->getType() == "window")
                    dynamic_pointer_cast<Window>(obj.second)->setAttribute("swapTest", args);
        });
        return true;
    });
    setAttributeDescription("swapTest", "Activate video swap test if set to 1");

    addAttribute("swapTestColor", [&](const Values& args) {
        addTask([=]() {
            lock_guard<recursive_mutex> lock(_objectsMutex);
            for (auto& obj : _objects)
                if (obj.second->getType() == "window")
                    dynamic_pointer_cast<Window>(obj.second)->setAttribute("swapTestColor", args);
        });
        return true;
    });
    setAttributeDescription("swapTestColor", "Set the swap test color");

    addAttribute("quit", [&](const Values& args) {
        addTask([=]() {
            _started = false;
            _isRunning = false;
        });
        return true;
    });
    setAttributeDescription("quit", "Ask the Scene to quit");

    addAttribute("unlink",
        [&](const Values& args) {
            addTask([=]() {
                string src = args[0].as<string>();
                string dst = args[1].as<string>();
                unlink(src, dst);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("unlink", "Unlink the two given objects");

    addAttribute("unlinkGhost",
        [&](const Values& args) {
            if (args.size() < 2)
                return false;

            addTask([=]() {
                string src = args[0].as<string>();
                string dst = args[1].as<string>();
                unlinkGhost(src, dst);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("unlinkGhost", "Unlink the two given ghost objects");

    addAttribute("wireframe",
        [&](const Values& args) {
            addTask([=]() {
                lock_guard<recursive_mutex> lock(_objectsMutex);
                for (auto& obj : _objects)
                    if (obj.second->getType() == "camera")
                        dynamic_pointer_cast<Camera>(obj.second)->setAttribute("wireframe", {(int)(args[0].as<int>())});
                for (auto& obj : _ghostObjects)
                    if (obj.second->getType() == "camera")
                        dynamic_pointer_cast<Camera>(obj.second)->setAttribute("wireframe", {(int)(args[0].as<int>())});
            });

            return true;
        },
        {'n'});
    setAttributeDescription("wireframe", "Show all meshes as wireframes if set to 1");

#if HAVE_GPHOTO
    addAttribute("calibrateColor", [&](const Values& args) {
        if (_colorCalibrator == nullptr)
            return false;
        // This needs to be launched in another thread, as the set mutex is already locked
        // (and we will need it later)
        SThread::pool.enqueue([&]() { _colorCalibrator->update(); });
        return true;
    });
    setAttributeDescription("calibrateColor", "Launch projectors color calibration");

    addAttribute("calibrateColorResponseFunction", [&](const Values& args) {
        if (_colorCalibrator == nullptr)
            return false;
        // This needs to be launched in another thread, as the set mutex is already locked
        // (and we will need it later)
        SThread::pool.enqueue([&]() { _colorCalibrator->updateCRF(); });
        return true;
    });
    setAttributeDescription("calibrateColorResponseFunction", "Launch the camera color calibration");
#endif

#if HAVE_LINUX
    addAttribute("sceneAffinity",
        [&](const Values& args) {
            auto core = args[0].as<int>();
            auto rootObjectCount = args[1].as<int>();

            addTask([=]() {
                if (Utils::setAffinity({core}))
                    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Set to run on CPU core #" << core << Log::endl;
                else
                    Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to set Scene CPU core affinity" << Log::endl;

                if (Utils::setRealTime())
                    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Set to realtime priority" << Log::endl;
                else
                    Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to set scheduling priority" << Log::endl;

                vector<int> cores{};
                for (int i = rootObjectCount; i < Utils::getCoreCount(); ++i)
                    cores.push_back(i);
                SThread::pool.setAffinity(cores);
            });

            return true;
        },
        {'n', 'n'});
    setAttributeDescription("sceneAffinity", "Set the core for the main loop, as well as the number of root objects. The thread pool will use the remaining cores.");
#endif

    addAttribute("configurationPath",
        [&](const Values& args) {
            _configurationPath = args[0].as<string>();
            return true;
        },
        {'s'});
    setAttributeDescription("configurationPath", "Path to the configuration files");

    addAttribute("mediaPath",
        [&](const Values& args) {
            _mediaPath = args[0].as<string>();
            return true;
        },
        {'s'});
    setAttributeDescription("mediaPath", "Path to the media files");

    addAttribute("runInBackground",
        [&](const Values& args) {
            _runInBackground = args[0].as<bool>();
            return true;
        },
        {'n'});
}

} // end of namespace
