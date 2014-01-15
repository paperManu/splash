#include "scene.h"
#include "timer.h"

using namespace std;

namespace Splash {

/*************/
Scene::Scene()
{
    init();

    _threadPool.reset(new ThreadPool(4));
}

/*************/
Scene::~Scene()
{
}

/*************/
BaseObjectPtr Scene::add(string type, string name)
{
    glfwMakeContextCurrent(_mainWindow->get());

    if (type == string("camera"))
    {
        CameraPtr camera(new Camera(_mainWindow));
        camera->setId(getId());
        if (name == string())
            _cameras[to_string(camera->getId())] = camera;
        else
            _cameras[name] = camera;
        return dynamic_pointer_cast<BaseObject>(camera);
    }
    else if (type == string("window"))
    {
        WindowPtr window(new Window(getNewSharedWindow()));
        window->setId(getId());
        if (name == string())
            _windows[to_string(window->getId())] = window;
        else
            _windows[name] = window;
        return dynamic_pointer_cast<BaseObject>(window);
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
    else if (type == string("geometry"))
    {
        GeometryPtr geometry(new Geometry());
        geometry->setId(getId());
        if (name == string())
            _geometries[to_string(geometry->getId())] = geometry;
        else
            _geometries[name] = geometry;
        return dynamic_pointer_cast<BaseObject>(geometry);
    }
    else if (type == string("mesh"))
    {
        MeshPtr mesh(new Mesh());
        mesh->setId(getId());
        if (name == string())
            _meshes[to_string(mesh->getId())] = mesh;
        else
            _meshes[name] = mesh;
        return dynamic_pointer_cast<BaseObject>(mesh);
    }
    else if (type == string("image") || type == string("image_shmdata"))
    {
        ImagePtr image(new Image());
        image->setId(getId());
        if (name == string())
            _images[to_string(image->getId())] = image;
        else
            _images[name] = image;
        return dynamic_pointer_cast<BaseObject>(image);
    }
    else if (type == string("texture"))
    {
        TexturePtr tex(new Texture());
        tex->setId(getId());
        if (name == string())
            _textures[to_string(tex->getId())] = tex;
        else
            _textures[name] = tex;
        return dynamic_pointer_cast<BaseObject>(tex);
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
    else if (_objects.find(second) != _objects.end())
        sink = _objects[second];

    if (_geometries.find(first) != _geometries.end())
        source = _geometries[first];
    else if (_geometries.find(second) != _geometries.end())
        sink = _geometries[second];

    if (_meshes.find(first) != _meshes.end())
        source = _meshes[first];
    else if (_meshes.find(second) != _meshes.end())
        sink = _meshes[second];

    if (_images.find(first) != _images.end())
        source = _images[first];
    else if (_images.find(second) != _images.end())
        sink = _images[second];

    if (_textures.find(first) != _textures.end())
        source = _textures[first];
    else if (_textures.find(second) != _textures.end())
        sink = _textures[second];

    if (_cameras.find(first) != _cameras.end())
        source = _cameras[first];
    else if (_cameras.find(second) != _cameras.end())
        sink = _cameras[second];

    if (_windows.find(first) != _windows.end())
        source = _windows[first];
    else if (_windows.find(second) != _windows.end())
        sink = _windows[second];

    if (source.get() != nullptr && sink.get() != nullptr)
        link(source, sink);
}

/*************/
bool Scene::link(BaseObjectPtr first, BaseObjectPtr second)
{
    glfwMakeContextCurrent(_mainWindow->get());

    if (dynamic_pointer_cast<Texture>(first).get() != nullptr)
    {
        if (dynamic_pointer_cast<Object>(second).get() != nullptr)
        {
            TexturePtr tex = dynamic_pointer_cast<Texture>(first);
            ObjectPtr obj = dynamic_pointer_cast<Object>(second);
            obj->addTexture(tex);
            return true;
        }
        else if (dynamic_pointer_cast<Window>(second).get() != nullptr)
        {
            TexturePtr tex = dynamic_pointer_cast<Texture>(first);
            WindowPtr win = dynamic_pointer_cast<Window>(second);
            win->setTexture(tex);
            return true;
        }
    }
    else if (dynamic_pointer_cast<Image>(first).get() != nullptr)
    {
        if (dynamic_pointer_cast<Texture>(second).get() != nullptr)
        {
            ImagePtr img = dynamic_pointer_cast<Image>(first);
            TexturePtr tex = dynamic_pointer_cast<Texture>(second);
            *tex = img;
            return true;
        }
    }
    else if (dynamic_pointer_cast<Object>(first).get() != nullptr)
    {
        if (dynamic_pointer_cast<Camera>(second).get() != nullptr)
        {
            ObjectPtr obj = dynamic_pointer_cast<Object>(first);
            CameraPtr cam = dynamic_pointer_cast<Camera>(second);
            cam->addObject(obj);
            return true;
        }
    }
    else if (dynamic_pointer_cast<Mesh>(first).get() != nullptr)
    {
        if (dynamic_pointer_cast<Geometry>(second).get() != nullptr)
        {
            MeshPtr mesh = dynamic_pointer_cast<Mesh>(first);
            GeometryPtr geom = dynamic_pointer_cast<Geometry>(second);
            geom->setMesh(mesh);
            return true;
        }
    }
    else if (dynamic_pointer_cast<Geometry>(first).get() != nullptr)
    {
        if (dynamic_pointer_cast<Object>(second).get() != nullptr)
        {
            GeometryPtr geom = dynamic_pointer_cast<Geometry>(first);
            ObjectPtr obj = dynamic_pointer_cast<Object>(second);
            obj->addGeometry(geom);
            return true;
        }
    }
    else if (dynamic_pointer_cast<Camera>(first).get() != nullptr)
    {
        if (dynamic_pointer_cast<Window>(second).get() != nullptr)
        {
            CameraPtr cam = dynamic_pointer_cast<Camera>(first);
            WindowPtr win = dynamic_pointer_cast<Window>(second);
            for (auto tex : cam->getTextures())
                win->setTexture(tex);
            return true;
        }
    }

    glfwMakeContextCurrent(NULL);

    return false;
}

/*************/
bool Scene::render()
{
    STimer::timer << "sceneTimer";
    bool isError {false};

    // Update the textures
    STimer::timer << "textures";
    glfwMakeContextCurrent(_mainWindow->get());
    for (auto& texture : _textures)
        texture.second->update();
    glfwMakeContextCurrent(0);
    STimer::timer >> "textures";

    // Update the cameras
    STimer::timer << "cameras";
    for (auto& camera : _cameras)
        isError |= camera.second->render();
    STimer::timer >> "cameras";

    // Update the windows
    STimer::timer << "windows";
    for (auto& window : _windows)
        _threadPool->enqueue([&]() {
            isError |= window.second->render();
        });
    _threadPool->waitAllThreads();
    STimer::timer >> "windows";

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

        if (key == GLFW_KEY_ESCAPE)
            quit = true;
    }
    STimer::timer >> "events";

    STimer::timer >> "sceneTimer";

    return quit;
}

/*************/
void Scene::setAttribute(string name, string attrib, std::vector<Value> args)
{
    if (_cameras.find(name) != _cameras.end())
        _cameras[name]->setAttribute(attrib, args);
    else if (_windows.find(name) != _windows.end())
        _windows[name]->setAttribute(attrib, args);
    else if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
    else if (_images.find(name) != _images.end())
        _images[name]->setAttribute(attrib, args);
    else if (_meshes.find(name) != _meshes.end())
        _meshes[name]->setAttribute(attrib, args);
}

/*************/
void Scene::setFromSerializedObject(const std::string name, const SerializedObject& obj)
{
    if (_images.find(name) != _images.end())
        _images[name]->deserialize(obj);
    else if (_meshes.find(name) != _meshes.end())
        _meshes[name]->deserialize(obj);
}

/*************/
GlWindowPtr Scene::getNewSharedWindow()
{
    if (!_mainWindow)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Main window does not exist, unable to create new shared window" << Log::endl;
        return GlWindowPtr(nullptr);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SPLASH_GL_CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SPLASH_GL_CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, SPLASH_GL_DEBUG);
    glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(512, 512, "Splash::Window", NULL, _mainWindow->get());
    if (!window)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
        return GlWindowPtr(nullptr);
    }
    return GlWindowPtr(new GlWindow(window, _mainWindow->get()));
}

/*************/
void Scene::init()
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

    GLFWwindow* window = glfwCreateWindow(512, 512, "Splash::Scene", NULL, NULL);

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
