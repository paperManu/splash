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
        CameraPtr camera(new Camera(getNewSharedWindow()));
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
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(512, 512, "sharedWindow", NULL, _mainWindow->get());
    if (!window)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
        return GlWindowPtr(nullptr);
    }
    return GlWindowPtr(new GlWindow(window));
}

/*************/
void Scene::init()
{
    // GLFW stuff
    if (!glfwInit())
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
        _isInitialized = false;
        return;
    }

    glfwSetErrorCallback(Scene::glfwErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SPLASH_GL_CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SPLASH_GL_CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, SPLASH_GL_DEBUG);
    glfwWindowHint(GLFW_VISIBLE, false);

    GLFWwindow* window = glfwCreateWindow(512, 512, "splash", NULL, NULL);

    if (!window)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        _isInitialized = false;
        return;
    }

    _mainWindow.reset(new GlWindow(window));
    glfwMakeContextCurrent(_mainWindow->get());
    _isInitialized = true;
    glfwMakeContextCurrent(NULL);
}

/*************/
void Scene::glfwErrorCallback(int code, const char* msg)
{
    SLog::log << Log::WARNING << msg << Log::endl;
}

/*************/
void Scene::render()
{
    for (auto camera : _cameras)
        camera.second->render();

    for (auto window : _windows)
        window.second->render();
}

} // end of namespace
