#include "scene.h"
#include "timer.h"

using namespace std;

namespace Splash {

/*************/
Scene::Scene(std::string name)
{
    init(name);
}

/*************/
Scene::~Scene()
{
    SLog::log << Log::DEBUG << "Scene::~Scene - Destructor" << Log::endl;
}

/*************/
BaseObjectPtr Scene::add(string type, string name)
{
    glfwMakeContextCurrent(_mainWindow->get());

    SLog::log << Log::DEBUG << "Scene::" << __FUNCTION__ << " - Creating object of type " << type << Log::endl;
    if (type == string("camera"))
    {
        CameraPtr camera(new Camera(_mainWindow));
        camera->setId(getId());
        if (name == string())
            _objects[to_string(camera->getId())] = camera;
        else
            _objects[name] = camera;
        return dynamic_pointer_cast<BaseObject>(camera);
    }
    else if (type == string("geometry"))
    {
        GeometryPtr geometry(new Geometry());
        geometry->setId(getId());
        if (name == string())
            _objects[to_string(geometry->getId())] = geometry;
        else
            _objects[name] = geometry;
        return dynamic_pointer_cast<BaseObject>(geometry);
    }
    else if (type == string("gui"))
    {
        GuiPtr gui(new Gui(getNewSharedWindow(name, true)));
        gui->setId(getId());
        if (name == string())
            _objects[to_string(gui->getId())] = gui;
        else
            _objects[name] = gui;
        return dynamic_pointer_cast<BaseObject>(gui);
    }
    else if (type == string("image") || type == string("image_shmdata"))
    {
        ImagePtr image(new Image());
        image->setId(getId());
        if (name == string())
            _objects[to_string(image->getId())] = image;
        else
            _objects[name] = image;
        return dynamic_pointer_cast<BaseObject>(image);
    }
    else if (type == string("mesh"))
    {
        MeshPtr mesh(new Mesh());
        mesh->setId(getId());
        if (name == string())
            _objects[to_string(mesh->getId())] = mesh;
        else
            _objects[name] = mesh;
        return dynamic_pointer_cast<BaseObject>(mesh);
    }
    else if (type == string("object"))
    {
        ObjectPtr object(new Object());
        object->setId(getId());
        if (name == string())
            _objects[to_string(object->getId())] = object;
        else
            _objects[name] = object;
        return dynamic_pointer_cast<BaseObject>(object);
    }
    else if (type == string("texture"))
    {
        TexturePtr tex(new Texture());
        tex->setId(getId());
        if (name == string())
            _objects[to_string(tex->getId())] = tex;
        else
            _objects[name] = tex;
        return dynamic_pointer_cast<BaseObject>(tex);
    }
    else if (type == string("window"))
    {
        WindowPtr window(new Window(getNewSharedWindow(name)));
        window->setId(getId());
        if (name == string())
            _objects[to_string(window->getId())] = window;
        else
            _objects[name] = window;
        return dynamic_pointer_cast<BaseObject>(window);
    }
    else
        return BaseObjectPtr();

    glfwMakeContextCurrent(NULL);
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
        link(source, sink);
}

/*************/
bool Scene::link(BaseObjectPtr first, BaseObjectPtr second)
{
    glfwMakeContextCurrent(_mainWindow->get());
    bool result = second->linkTo(first);
    glfwMakeContextCurrent(NULL);

    return result;
}

/*************/
bool Scene::render()
{
    STimer::timer << "sceneTimer";
    bool isError {false};

    // Update the cameras and the guis
    STimer::timer << "cameras";
    for (auto& obj : _objects)
        if (obj.second->getType() == "camera")
            isError |= dynamic_pointer_cast<Camera>(obj.second)->render();
        else if (obj.second->getType() == "gui")
            isError |= dynamic_pointer_cast<Gui>(obj.second)->render();
    STimer::timer >> "cameras";

    // Update the windows
    STimer::timer << "windows";
    vector<unsigned int> threadIds;
    for (auto& obj : _objects)
        if (obj.second->getType() == "window")
            threadIds.push_back(SThread::pool.enqueue([&]() {
                isError |= dynamic_pointer_cast<Window>(obj.second)->render();
            }));
    SThread::pool.waitThreads(threadIds);
    STimer::timer >> "windows";

    // Swap all buffers at once
    STimer::timer << "swap";
    threadIds.clear();
    for (auto& obj : _objects)
        if (obj.second->getType() == "window")
            threadIds.push_back(SThread::pool.enqueue([&]() {
                dynamic_pointer_cast<Window>(obj.second)->swapBuffers();
            }));
    SThread::pool.waitThreads(threadIds);
    STimer::timer >> "swap";

    _status = !isError;

    // Update the user events
    STimer::timer << "events";
    bool quit = false;
    glfwPollEvents();
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
                if (dynamic_pointer_cast<Window>(w.second)->isWindow(win))
                    eventWindow = dynamic_pointer_cast<Window>(w.second);

        if (key == GLFW_KEY_ESCAPE)
            quit = true;
        else if (key == GLFW_KEY_F)
            if (mods == GLFW_MOD_ALT && action == GLFW_PRESS)
                if (eventWindow.get() != nullptr)
                    eventWindow->switchFullscreen();

        // Send the action to the GUI
        for (auto& obj : _objects)
            if (obj.second->getType() == "gui")
                dynamic_pointer_cast<Gui>(obj.second)->key(key, action, mods);
    }
    STimer::timer >> "events";

    STimer::timer >> "sceneTimer";

    return quit;
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
void Scene::setAttribute(string name, string attrib, std::vector<Value> args)
{
    if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
}

/*************/
void Scene::setFromSerializedObject(const std::string name, const SerializedObject& obj)
{
    if (_objects.find(name) != _objects.end() && _objects[name]->getType() == "image")
        dynamic_pointer_cast<Image>(_objects[name])->deserialize(obj);
    else if (_objects.find(name) != _objects.end() && _objects[name]->getType() == "mesh")
        dynamic_pointer_cast<Mesh>(_objects[name])->deserialize(obj);
}

/*************/
GlWindowPtr Scene::getNewSharedWindow(string name, bool gl2)
{
    string windowName;
    name.size() == 0 ? windowName = "Splash::Window" : windowName = name;

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
    return GlWindowPtr(new GlWindow(window, _mainWindow->get()));
}

/*************/
void Scene::init(std::string name)
{
    glfwSetErrorCallback(Scene::glfwErrorCallback);

    // GLFW stuff
    if (!glfwInit())
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
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
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        _isInitialized = false;
        return;
    }

    _mainWindow.reset(new GlWindow(window, window));
    glfwMakeContextCurrent(_mainWindow->get());
    _isInitialized = true;
    glfwMakeContextCurrent(NULL);
}

/*************/
void Scene::glfwErrorCallback(int code, const char* msg)
{
    SLog::log << Log::WARNING << "Scene::glfwErrorCallback - " << msg << Log::endl;
}

} // end of namespace
