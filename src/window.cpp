#include "window.h"
#include "camera.h"
#include "gui.h"

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace std::placeholders;

namespace Splash {

/*************/
mutex Window::_callbackMutex;
deque<pair<GLFWwindow*, vector<int>>> Window::_keys;
deque<pair<GLFWwindow*, vector<int>>> Window::_mouseBtn;
pair<GLFWwindow*, vector<double>> Window::_mousePos;

/*************/
Window::Window(GlWindowPtr w)
{
    _type = "window";

    if (w.get() == nullptr)
        return;

    _window = w;
    _isInitialized = setProjectionSurface();
    if (!_isInitialized)
        SLog::log << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while creating the Window" << Log::endl;
    else
        SLog::log << Log::MESSAGE << "Window::" << __FUNCTION__ << " - Window created successfully" << Log::endl;

    registerAttributes();
}

/*************/
Window::~Window()
{
}

/*************/
bool Window::getKey(int key)
{
    if (glfwGetKey(_window->get(), key) == GLFW_PRESS)
        return true;
}

/*************/
int Window::getKeys(GLFWwindow*& win, int& key, int& action, int& mods)
{
    if (_keys.size() == 0)
        return 0;

    win = _keys.front().first;
    vector<int> keys = _keys.front().second;

    key = keys[0];
    action = keys[2];
    mods = keys[3];

    _keys.pop_front();

    return _keys.size() + 1;
}

/*************/
int Window::getMouseBtn(GLFWwindow* win, int& btn, int& action, int& mods)
{
    if (_mouseBtn.size() == 0)
        return 0;

    win = _mouseBtn.front().first;
    vector<int> mouse = _mouseBtn.front().second;

    btn = mouse[0];
    action = mouse[1];
    mods = mouse[2];

    _mouseBtn.pop_front();

    return _mouseBtn.size() + 1;
}

/*************/
void Window::getMousePos(GLFWwindow* win, double xpos, double ypos)
{
    win = _mousePos.first;
    xpos = _mousePos.second[0];
    ypos = _mousePos.second[1];
}

/*************/
bool Window::linkTo(BaseObjectPtr obj)
{
    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        TexturePtr tex = dynamic_pointer_cast<Texture>(obj);
        _isLinkedToTexture = true;
        setTexture(tex);
        return true;
    }
    else if (dynamic_pointer_cast<Camera>(obj).get() != nullptr)
    {
        CameraPtr cam = dynamic_pointer_cast<Camera>(obj);
        for (auto& tex : cam->getTextures())
            setTexture(tex);
        return true;
    }
    else if (dynamic_pointer_cast<Gui>(obj).get() != nullptr)
    {
        GuiPtr gui = dynamic_pointer_cast<Gui>(obj);
        setTexture(gui->getTexture());
        return true;
    }

    return false;
}

/*************/
bool Window::render()
{
    glfwMakeContextCurrent(_window->get());

    int w, h;
    glfwGetWindowSize(_window->get(), &w, &h);
    glViewport(0, 0, w, h);

    glGetError();
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT);

    _screen->activate();
    _screen->setViewProjectionMatrix(glm::ortho(-1.f, 1.f, -1.f, 1.f));
    _screen->draw();
    _screen->deactivate();

    // Resize the input textures accordingly to the window size.
    // This goes upward to the cameras and gui
    if (!_isLinkedToTexture) // We don't do this if we are directly connected to a Texture (updated from an image)
        for (auto& t : _inTextures)
            t->resize(w, h);

    GLenum error = glGetError();
    if (error)
        SLog::log << Log::WARNING << _type << "::" << __FUNCTION__ << " - Error while rendering the window: " << error << Log::endl;

    glfwMakeContextCurrent(NULL);

    return error != 0 ? true : false;
}

/*************/
void Window::swapBuffers()
{
    glfwSwapBuffers(_window->get());
}

/*************/
bool Window::switchFullscreen(int screenId)
{
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (screenId >= count)
        return false;

    if (_window.get() == nullptr)
        return false;

    if (screenId != -1)
        _screenId = screenId;

    const GLFWvidmode* vidmode = glfwGetVideoMode(monitors[_screenId]);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SPLASH_GL_CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SPLASH_GL_CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, SPLASH_GL_DEBUG);
    glfwWindowHint(GLFW_VISIBLE, true);
    GLFWwindow* window;
    if (glfwGetWindowMonitor(_window->get()) == NULL)
        window = glfwCreateWindow(vidmode->width, vidmode->height, to_string(_screenId).c_str(), monitors[_screenId], _window->getMainWindow());
    else
        window = glfwCreateWindow(vidmode->width, vidmode->height, to_string(_screenId).c_str(), 0, _window->getMainWindow());

    if (!window)
    {
        SLog::log << Log::WARNING << "Window::" << __FUNCTION__ << " - Unable to create new fullscreen shared window" << Log::endl;
        return false;
    }

    _window = move(GlWindowPtr(new GlWindow(window, _window->getMainWindow())));

    setProjectionSurface();
    for (auto t : _inTextures)
        _screen->addTexture(t);

    return true;
}

/*************/
void Window::setTexture(TexturePtr tex)
{
    bool isPresent = false;
    for (auto t : _inTextures)
        if (tex == t)
            isPresent = true;

    if (isPresent)
        return;

    _inTextures.push_back(tex);
    _screen->addTexture(tex);
}

/*************/
void Window::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    std::vector<int> keys {key, scancode, action, mods};
    _keys.push_back(pair<GLFWwindow*, vector<int>>(win,keys));
}

/*************/
void Window::mouseBtnCallback(GLFWwindow* win, int button, int action, int mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    std::vector<int> btn {button, action, mods};
    _mouseBtn.push_back(pair<GLFWwindow*, vector<int>>(win,btn));
}

/*************/
void Window::mousePosCallback(GLFWwindow* win, double xpos, double ypos)
{
    lock_guard<mutex> lock(_callbackMutex);
    std::vector<double> pos {xpos, ypos};
    _mousePos.first = win;
    _mousePos.second = move(pos);
}

/*************/
void Window::registerAttributes()
{
    _attribFunctions["fullscreen"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        switchFullscreen(args[0].asInt());
        return true;
    });
}

/*************/
bool Window::setProjectionSurface()
{
    glfwMakeContextCurrent(_window->get());
    glfwShowWindow(_window->get());
    glfwSwapInterval(SPLASH_SWAP_INTERVAL);

    // Setup the projection surface
    glGetError();

    _screen.reset(new Object());
    GeometryPtr virtualScreen(new Geometry());
    _screen->addGeometry(virtualScreen);
    ShaderPtr shader(new Shader());
    _screen->setShader(shader);

    GLenum error = glGetError();
    if (error)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Error while creating the projection surface: " << error << Log::endl;
        return false;
    }

    glfwMakeContextCurrent(0);

    // Initialize the callbacks
    glfwSetKeyCallback(_window->get(), Window::keyCallback);
    glfwSetMouseButtonCallback(_window->get(), Window::mouseBtnCallback);
    glfwSetCursorPosCallback(_window->get(), Window::mousePosCallback);

    return true;
}

} // end of namespace
