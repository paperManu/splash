#include "scene.h"

#include "camera.h"
#include "geometry.h"
#include "gui.h"
#include "image.h"
#include "link.h"
#include "log.h"
#include "mesh.h"
#include "mesh_shmdata.h"
#include "object.h"
#include "texture.h"
#include "threadpool.h"
#include "timer.h"
#include "window.h"

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#include <GL/glxext.h>

using namespace std;
using namespace OIIO_NAMESPACE;

namespace Splash {

/*************/
Scene::Scene(std::string name)
{
    _self = ScenePtr(this, [](Scene*){}); // A shared pointer with no deleter, how convenient

    SLog::log << Log::DEBUGGING << "Scene::Scene - Scene created successfully" << Log::endl;

    _isRunning = true;
    _name = name;
    _sceneLoop = thread([&]() {
        init(_name);
        registerAttributes();
        run();
    });
}

/*************/
Scene::~Scene()
{
    SLog::log << Log::DEBUGGING << "Scene::~Scene - Destructor" << Log::endl;
    _sceneLoop.join();
}

/*************/
BaseObjectPtr Scene::add(string type, string name)
{
    SLog::log << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - Creating object of type " << type << Log::endl;

    unique_lock<mutex> lock(_configureMutex);

    BaseObjectPtr obj;
    // First, the objects containing a context
    if (type == string("camera"))
        obj = dynamic_pointer_cast<BaseObject>(make_shared<Camera>(_mainWindow));
    else if (type == string("window"))
    {
        obj = dynamic_pointer_cast<BaseObject>(make_shared<Window>(getNewSharedWindow(name)));
        obj->setAttribute("swapInterval", {_swapInterval});
    }
    else if (type == string("gui"))
        obj = dynamic_pointer_cast<BaseObject>(make_shared<Gui>(getNewSharedWindow(name, true), _self));
    else
    {
        // Then, the objects not containing a context
        _mainWindow->setAsCurrentContext();
        if (type == string("geometry"))
            obj = dynamic_pointer_cast<BaseObject>(make_shared<Geometry>());
        else if (type == string("image") || type == string("image_shmdata"))
        {
            obj = dynamic_pointer_cast<BaseObject>(make_shared<Image>());
            obj->setRemoteType(type);
        }
        else if (type == string("mesh"))
            obj = dynamic_pointer_cast<BaseObject>(make_shared<Mesh>());
        else if (type == string("mesh_shmdata"))
            obj = dynamic_pointer_cast<BaseObject>(make_shared<Mesh_Shmdata>());
        else if (type == string("object"))
            obj = dynamic_pointer_cast<BaseObject>(make_shared<Object>());
        else if (type == string("texture"))
            obj = dynamic_pointer_cast<BaseObject>(make_shared<Texture>());
        _mainWindow->releaseContext();
    }

    if (obj.get() != nullptr)
    {
        obj->setId(getId());
        obj->setName(name);
        if (name == string())
            _objects[to_string(obj->getId())] = obj;
        else
            _objects[name] = obj;
    }

    return obj;
}

/*************/
void Scene::addGhost(string type, string name)
{

    // Currently, only Cameras can be ghosts
    if (type != string("camera"))
        return;

    SLog::log << Log::DEBUGGING << "Scene::" << __FUNCTION__ << " - Creating ghost object of type " << type << Log::endl;

    // Add the object for real ...
    BaseObjectPtr obj = add(type, name);

    // And move it to _ghostObjects
    unique_lock<mutex> lock(_configureMutex);
    _objects.erase(obj->getName());
    _ghostObjects[obj->getName()] = obj;
}

/*************/
Json::Value Scene::getConfigurationAsJson()
{
    unique_lock<mutex> lock(_configureMutex);

    Json::Value root;

    for (auto& obj : _objects)
        root[obj.first] = obj.second->getConfigurationAsJson();

    return root;
}

/*************/
bool Scene::link(string first, string second)
{
    BaseObjectPtr source(nullptr);
    BaseObjectPtr sink(nullptr);

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
bool Scene::link(BaseObjectPtr first, BaseObjectPtr second)
{
    unique_lock<mutex> lock(_configureMutex);

    glfwMakeContextCurrent(_mainWindow->get());
    bool result = second->linkTo(first);
    glfwMakeContextCurrent(NULL);

    return result;
}

/*************/
bool Scene::linkGhost(string first, string second)
{
    BaseObjectPtr source(nullptr);
    BaseObjectPtr sink(nullptr);

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

    // TODO: add a mechanism in objects to check if already linked
    if (source->getType() != "camera" && sink->getType() != "camera")
        return false;

    return link(source, sink);
}

/*************/
void Scene::remove(string name)
{
    BaseObjectPtr obj;

    if (_objects.find(name) != _objects.end())
        _objects.erase(name);
    else if (_ghostObjects.find(name) != _ghostObjects.end())
        _ghostObjects.erase(name);
}

/*************/
void Scene::render()
{
    if (!_started)
        return;

    bool isError {false};
    vector<unsigned int> threadIds;

    // Swap all buffers at once
    STimer::timer << "swap";
    for (auto& obj : _objects)
        if (obj.second->getType() == "window")
            threadIds.push_back(SThread::pool.enqueue([&]() {
                dynamic_pointer_cast<Window>(obj.second)->swapBuffers();
            }));
    _status = !isError;
    
    // Update the cameras
    STimer::timer << "cameras";
    for (auto& obj : _objects)
        if (obj.second->getType() == "camera")
            isError |= dynamic_pointer_cast<Camera>(obj.second)->render();
    STimer::timer >> "cameras";

    // Update the guis
    STimer::timer << "guis";
    for (auto& obj : _objects)
        if (obj.second->getType() == "gui")
            isError |= dynamic_pointer_cast<Gui>(obj.second)->render();
    STimer::timer >> "guis";

    // Wait for buffer update and swap threads
    SThread::pool.waitThreads(threadIds);
    STimer::timer >> "swap";

    // Update the windows
    STimer::timer << "windows";
    for (auto& obj : _objects)
        if (obj.second->getType() == "window")
            isError |= dynamic_pointer_cast<Window>(obj.second)->render();
    STimer::timer >> "windows";

    // Update the user events
    glfwPollEvents();
    // Mouse position
    {
        GLFWwindow* win;
        int xpos, ypos;
        Window::getMousePos(win, xpos, ypos);
        for (auto& obj : _objects)
            if (obj.second->getType() == "gui")
                dynamic_pointer_cast<Gui>(obj.second)->mousePosition(xpos, ypos);
    }

    // Mouse events
    while (true)
    {
        GLFWwindow* win;
        int btn, action, mods;
        if (!Window::getMouseBtn(win, btn, action, mods))
            break;
        
        for (auto& obj : _objects)
            if (obj.second->getType() == "gui")
                dynamic_pointer_cast<Gui>(obj.second)->mouseButton(btn, action, mods);
    }

    // Scrolling events
    while (true)
    {
        GLFWwindow* win;
        double xoffset, yoffset;
        if (!Window::getScroll(win, xoffset, yoffset))
            break;

        for (auto& obj : _objects)
            if (obj.second->getType() == "gui")
                dynamic_pointer_cast<Gui>(obj.second)->mouseScroll(xoffset, yoffset);
    }

    // Keyboard events
    while (true)
    {
        GLFWwindow* win;
        int key, action, mods;
        if (!Window::getKeys(win, key, action, mods))
            break;

        // Find where this action happened
        WindowPtr eventWindow;
        for (auto& w : _objects)
            if (w.second->getType() == "window")
            {
                WindowPtr window = dynamic_pointer_cast<Window>(w.second);
                if (window->isWindow(win))
                    eventWindow = window;
            }

        if (key == GLFW_KEY_F)
        {
            if (mods == GLFW_MOD_ALT && action == GLFW_PRESS)
                if (eventWindow.get() != nullptr)
                {
                    eventWindow->switchFullscreen();
                    continue;
                }
        }

        // Send the action to the GUI
        for (auto& obj : _objects)
            if (obj.second->getType() == "gui")
                dynamic_pointer_cast<Gui>(obj.second)->key(key, action, mods);
    }
}

/*************/
void Scene::run()
{
    while (_isRunning)
    {
        STimer::timer << "sceneLoop";
        if (_started)
        {
            unique_lock<mutex> lock(_configureMutex);
            render();
        }

        // Saving event
        if (_doSaveNow)
        {
            setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas
            Json::Value config = getConfigurationAsJson();
            string configStr = config.toStyledString();
            sendMessage("sceneConfig", {_name, configStr});
            _doSaveNow = false;
        }
        
        // Compute blending event
        if (_doComputeBlending)
        {
            computeBlendingMap();
            _doComputeBlending = false;
        }

        STimer::timer >> "sceneLoop";
    }
}

/*************/
void Scene::setAsWorldScene()
{
    // First we create a single camera linked to a single window
    add("camera", "_camera");
    add("window", "_window");
    link("_camera", "_window");

    // Then we need to connect all Objects to the camera
    for (auto& obj : _objects)
        link(obj.first, "_camera");
}

/*************/
void Scene::sendMessage(const string message, const vector<Value> value)
{
    _link->sendMessage("world", message, value);
}

/*************/
void Scene::computeBlendingMap()
{
    if (_isBlendComputed)
    {
        for (auto& obj : _objects)
            if (obj.second->getType() == "object")
                dynamic_pointer_cast<Object>(obj.second)->resetBlendingMap();

        _isBlendComputed = false;

        SLog::log << "Scene::" << __FUNCTION__ << " - Camera blending deactivated" << Log::endl;
    }
    else
    {
        initBlendingMap();
        // Set the blending map to zero
        _blendingMap->setTo(0.f);
        _blendingMap->setName("blendingMap");

        // Compute the contribution of each camera
        for (auto& obj : _objects)
            if (obj.second->getType() == "camera")
                dynamic_pointer_cast<Camera>(obj.second)->computeBlendingMap(_blendingMap);
        for (auto& obj : _ghostObjects)
            if (obj.second->getType() == "camera")
                dynamic_pointer_cast<Camera>(obj.second)->computeBlendingMap(_blendingMap);

        // Filter the output to fill the blanks (dilate filter)
        ImagePtr buffer = make_shared<Image>(_blendingMap->getSpec());
        unsigned short* pixBuffer = (unsigned short*)buffer->data();
        unsigned short* pixels = (unsigned short*)_blendingMap->data();
        int w = _blendingMap->getSpec().width;
        int h = _blendingMap->getSpec().height;
        for (int x = 0; x < w; ++x)
            for (int y = 0; y < h; ++y)
            {
                unsigned short maxValue = 0;
                for (int xx = -1; xx <= 1; ++xx)
                {
                    if (x + xx < 0 || x + xx >= w)
                        continue;
                    for (int yy = -1; yy <= 1; ++yy)
                    {
                        if (y + yy < 0 || y + yy >= h)
                            continue;
                        maxValue = std::max(maxValue, pixels[(y + yy) * w + x + xx]);
                    }
                }
                pixBuffer[y * w + x] = maxValue;
            }
        swap(*_blendingMap, *buffer);
        _blendingMap->updateTimestamp();

        // Small hack to handle the fact that texture transfer uses PBOs.
        // If we send the buffer only once, the displayed PBOs wont be the correct one.
        if (_isMaster)
        {
            _link->sendBuffer("blendingMap", _blendingMap->serialize());
            timespec nap {0, (long int)1e8};
            nanosleep(&nap, NULL);
            _link->sendBuffer("blendingMap", _blendingMap->serialize());
        }

        for (auto& obj : _objects)
            if (obj.second->getType() == "object")
                dynamic_pointer_cast<Object>(obj.second)->setBlendingMap(_blendingTexture);

        _isBlendComputed = true;

        SLog::log << "Scene::" << __FUNCTION__ << " - Camera blending computed" << Log::endl;
    }
}

/*************/
GlWindowPtr Scene::getNewSharedWindow(string name, bool gl2)
{
    string windowName;
    name.size() == 0 ? windowName = "Splash::Window" : windowName = "Splash::" + name;

    if (!_mainWindow)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Main window does not exist, unable to create new shared window" << Log::endl;
        return GlWindowPtr(nullptr);
    }

    if (!gl2)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SPLASH_GL_CONTEXT_VERSION_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SPLASH_GL_CONTEXT_VERSION_MINOR);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    else
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    }
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, SPLASH_GL_DEBUG);
    glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(512, 512, windowName.c_str(), NULL, _mainWindow->get());
    if (!window)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
        return GlWindowPtr(nullptr);
    }

    glfwMakeContextCurrent(window);
    if (SPLASH_GL_DEBUG)
    {
        glDebugMessageCallback(Scene::glMsgCallback, (void*)this);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
    }

#ifdef GLX_NV_swap_group
    if (_maxSwapGroups)
    {
        bool nvResult = true;
        nvResult &= glXJoinSwapGroupNV(glfwGetX11Display(), glfwGetX11Window(window), 1);
        nvResult &= glXBindSwapBarrierNV(glfwGetX11Display(), 1, 1);
        if (nvResult)
            SLog::log << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << name << " successfully joined the NV swap group" << Log::endl;
        else
            SLog::log << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Window " << name << " couldn't join the NV swap group" << Log::endl;
    }
#endif

    glfwMakeContextCurrent(NULL);

    return make_shared<GlWindow>(window, _mainWindow->get());
}

/*************/
void Scene::init(std::string name)
{
    glfwSetErrorCallback(Scene::glfwErrorCallback);

    // GLFW stuff
    if (!glfwInit())
    {
        SLog::log << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
        _isInitialized = false;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SPLASH_GL_CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SPLASH_GL_CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, SPLASH_GL_DEBUG);
    glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_VISIBLE, false);

    GLFWwindow* window = glfwCreateWindow(512, 512, name.c_str(), NULL, NULL);

    if (!window)
    {
        SLog::log << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        _isInitialized = false;
        return;
    }

    _mainWindow = make_shared<GlWindow>(window, window);
    _isInitialized = true;

    glfwMakeContextCurrent(_mainWindow->get());
    // Activate GL debug messages
    if (SPLASH_GL_DEBUG)
    {
        glDebugMessageCallback(Scene::glMsgCallback, (void*)this);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
    }

    // Check for swap groups
#ifdef GLX_NV_swap_group
    if (!glXQueryMaxSwapGroupsNV(glfwGetX11Display(), 0, &_maxSwapGroups, &_maxSwapBarriers))
        SLog::log << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Unable to get NV max swap groups / barriers" << Log::endl;
    else
        SLog::log << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - NV max swap groups: " << _maxSwapGroups << " / barriers: " << _maxSwapBarriers << Log::endl;
#endif

    glfwMakeContextCurrent(NULL);

    // Create the link and connect to the World
    _link = make_shared<Link>(weak_ptr<Scene>(_self), name);
    _link->connectTo("world");
    _link->sendMessage("world", "childProcessLaunched", {});
}

/*************/
void Scene::initBlendingMap()
{
    _blendingMap = make_shared<Image>();
    _blendingMap->set(2048, 2048, 1, TypeDesc::UINT16);
    _objects["blendingMap"] = _blendingMap;

    glfwMakeContextCurrent(_mainWindow->get());
    _blendingTexture = make_shared<Texture>();
    _blendingTexture->disableFiltering();
    *_blendingTexture = _blendingMap;
    glfwMakeContextCurrent(NULL);
}

/*************/
void Scene::glfwErrorCallback(int code, const char* msg)
{
    SLog::log << Log::WARNING << "Scene::glfwErrorCallback - " << msg << Log::endl;
}

/*************/
void Scene::glMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    string typeString {""};
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        typeString = "Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeString = "Deprecated behavior";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeString = "Undefined behavior";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeString = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeString = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeString = "Other";
        break;
    }

    SLog::log << Log::WARNING << "GL::debug - [" << typeString << "] - " << message << Log::endl;
}

/*************/
void Scene::registerAttributes()
{
    _attribFunctions["add"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        string type = args[0].asString();
        string name = args[1].asString();

        add(type, name);
        return true;
    });

    _attribFunctions["addGhost"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        string type = args[0].asString();
        string name = args[1].asString();

        addGhost(type, name);
        return true;
    });

    _attribFunctions["computeBlending"] = AttributeFunctor([&](vector<Value> args) {
        _doComputeBlending = true;
        return true;
    });

    _attribFunctions["config"] = AttributeFunctor([&](vector<Value> args) {
        _doSaveNow = true;
        return true;
    });

    _attribFunctions["duration"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        STimer::timer.setDuration(args[0].asString(), args[1].asInt());
        return true;
    });
 
    _attribFunctions["flashBG"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        for (auto& obj : _objects)
            if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
                dynamic_pointer_cast<Camera>(obj.second)->setAttribute("flashBG", {(int)(args[0].asInt())});
        return true;
    });
   
    _attribFunctions["link"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        string src = args[0].asString();
        string dst = args[1].asString();
        link(src, dst);
        return true;
    });

    _attribFunctions["linkGhost"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        string src = args[0].asString();
        string dst = args[1].asString();
        return linkGhost(src, dst);
    });

    _attribFunctions["log"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        SLog::log.setLog(args[0].asString(), (Log::Priority)args[1].asInt());
        return true;
    });

    _attribFunctions["remove"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        string name = args[1].asString();

        remove(name);
        return true;
    });

    _attribFunctions["setGhost"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        string name = args[0].asString();
        string attr = args[1].asString();
        Values values;
        for (int i = 2; i < args.size(); ++i)
            values.push_back(args[i]);

        if (_ghostObjects.find(name) != _ghostObjects.end())
            _ghostObjects[name]->setAttribute(attr, values);

        return true;
    });

    _attribFunctions["setMaster"] = AttributeFunctor([&](vector<Value> args) {
        setAsMaster();
        return true;
    });

    _attribFunctions["start"] = AttributeFunctor([&](vector<Value> args) {
        _started = true;
        return true;
    });

    _attribFunctions["stop"] = AttributeFunctor([&](vector<Value> args) {
        _started = false;
        return true;
    });

    _attribFunctions["swapInterval"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _swapInterval = max(-1, args[0].asInt());
        return true;
    });

    _attribFunctions["quit"] = AttributeFunctor([&](vector<Value> args) {
        _started = false;
        _isRunning = false;
        return true;
    });
 
    _attribFunctions["wireframe"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        for (auto& obj : _objects)
            if (obj.second->getType() == "camera")
                dynamic_pointer_cast<Camera>(obj.second)->setAttribute("wireframe", {(int)(args[0].asInt())});
        for (auto& obj : _ghostObjects)
            if (obj.second->getType() == "camera")
                dynamic_pointer_cast<Camera>(obj.second)->setAttribute("wireframe", {(int)(args[0].asInt())});
        return true;
    });
}

} // end of namespace
