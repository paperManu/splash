#include "scene.h"

using namespace std;

namespace Splash {

/*************/
Scene::Scene()
{
    init();
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
    else if (type == string("image"))
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
bool Scene::link(BaseObjectPtr first, BaseObjectPtr second)
{
    glfwMakeContextCurrent(_mainWindow->get());

    if (first->getType() == string("texture"))
    {
        if (second->getType() == string("object"))
        {
            TexturePtr tex = static_pointer_cast<Texture>(first);
            ObjectPtr obj = static_pointer_cast<Object>(second);
            obj->addTexture(tex);
            return true;
        }
        else if (second->getType() == string("window"))
        {
            TexturePtr tex = static_pointer_cast<Texture>(first);
            WindowPtr win = static_pointer_cast<Window>(second);
            win->setTexture(tex);
            return true;
        }
    }
    else if (first->getType() == string("image"))
    {
        if (second->getType() == string("texture"))
        {
            ImagePtr img = static_pointer_cast<Image>(first);
            TexturePtr tex = static_pointer_cast<Texture>(second);
            *tex = img->get();
            return true;
        }
    }
    else if (first->getType() == string("object"))
    {
        if (second->getType() == string("camera"))
        {
            ObjectPtr obj = static_pointer_cast<Object>(first);
            CameraPtr cam = static_pointer_cast<Camera>(second);
            cam->addObject(obj);
            return true;
        }
    }
    else if (first->getType() == string("mesh"))
    {
        if (second->getType() == string("geometry"))
        {
            MeshPtr mesh = static_pointer_cast<Mesh>(first);
            GeometryPtr geom = static_pointer_cast<Geometry>(second);
            geom->setMesh(mesh);
            return true;
        }
    }
    else if (first->getType() == string("geometry"))
    {
        if (second->getType() == string("object"))
        {
            GeometryPtr geom = static_pointer_cast<Geometry>(first);
            ObjectPtr obj = static_pointer_cast<Object>(second);
            obj->addGeometry(geom);
            return true;
        }
    }
    else if (first->getType() == string("camera"))
    {
        if (second->getType() == string("window"))
        {
            CameraPtr cam = static_pointer_cast<Camera>(first);
            WindowPtr win = static_pointer_cast<Window>(second);
            for (auto tex : cam->getTextures())
                win->setTexture(tex);
            return true;
        }
    }

    glfwMakeContextCurrent(NULL);

    return false;
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
    GLFWwindow* window = glfwCreateWindow(512, 512, "sharedWindow", NULL, _mainWindow->get());
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

    GLFWwindow* window = glfwCreateWindow(512, 512, "splash", NULL, NULL);

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
    SLog::log << Log::WARNING << "Scene - " << msg << Log::endl;
}

/*************/
bool Scene::render()
{
    bool isError {false};
    // Update the cameras
    for (auto camera : _cameras)
        isError |= camera.second->render();

    // Update the windows
    for (auto window : _windows)
        isError |= window.second->render();

    _status = !isError;

    // Update the user events
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

    return quit;
}

/*************/
void Scene::setAttribute(string name, string attrib, std::vector<float> args)
{
    if (_cameras.find(name) != _cameras.end())
        _cameras[name]->setAttribute(attrib, args);
    else if (_windows.find(name) != _windows.end())
        _windows[name]->setAttribute(attrib, args);
    else if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
}

} // end of namespace
