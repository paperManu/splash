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
}

/*************/
GlWindowPtr Scene::getNewSharedWindow()
{
    if (!_mainWindow)
    {
        gLog << Log::WARNING << __FUNCTION__ << " - Main window does not exist, unable to create new shared window" << Log::endl;
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
        gLog << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
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
        gLog << Log::WARNING << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
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
        gLog << Log::WARNING << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
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
    gLog << Log::WARNING << msg << Log::endl;
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
