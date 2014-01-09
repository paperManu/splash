#include "window.h"

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace std::placeholders;

namespace Splash {

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
    glfwMakeContextCurrent(_window->get());
    glfwShowWindow(w->get());
    glfwSwapInterval(0);


    // Setup the projection surface
    glGetError();

    _screen.reset(new Object());
    GeometryPtr virtualScreen(new Geometry());
    _screen->addGeometry(virtualScreen);
    ShaderPtr shader(new Shader());
    _screen->setShader(shader);

    GLenum error = glGetError();
    if (error)
        SLog::log << Log::WARNING << __FUNCTION__ << " - Error while creating the window: " << error << Log::endl;

    // Initialize the callbacks
    if (!glfwSetKeyCallback(_window->get(), Window::keyCallback))
        SLog::log << Log::ERROR << "Window::" << __FUNCTION__ << " - Error while setting up key callback" << Log::endl;
    if (!glfwSetMouseButtonCallback(_window->get(), Window::mouseBtnCallback))
        SLog::log << Log::ERROR << "Window::" << __FUNCTION__ << " - Error while setting up mouse button callback" << Log::endl;
    if (!glfwSetCursorPosCallback(_window->get(), Window::mousePosCallback))
        SLog::log << Log::ERROR << "Window::" << __FUNCTION__ << " - Error while setting up mouse position callback" << Log::endl;

    glfwMakeContextCurrent(NULL);

    _isInitialized = true;
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
int Window::getKeys(GLFWwindow* win, int& key, int& action, int& mods)
{
    if (_keys.size() == 0)
        return 0;

    win = _keys.front().first;
    vector<int> keys = _keys.front().second;

    key = keys[0];
    action = keys[1];
    mods = keys[2];

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
void Window::render()
{
    glfwMakeContextCurrent(_window->get());

    // Print the pixel values...
    if (false && _inTextures.size() > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _inTextures[0]->getTexId());
        ImageBuf img(_inTextures[0]->getSpec());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, img.localpixels());
        for (ImageBuf::Iterator<unsigned char> p(img); !p.done(); ++p)
        {
            if (!p.exists())
                continue;
            for (int c = 0; c < img.nchannels(); ++c)
                cout << p[c] << " ";
        }
        cout << endl;
    }
    //

    int w, h;
    glfwGetWindowSize(_window->get(), &w, &h);
    glViewport(0, 0, w, h);

    glGetError();
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT);

    _screen->activate();
    _screen->setViewProjectionMatrix(glm::ortho(-1.1f, 1.1f, -1.1f, 1.1f));
    _screen->draw();
    _screen->deactivate();

    glfwSwapBuffers(_window->get());

    GLenum error = glGetError();
    if (error)
        SLog::log << Log::WARNING << _type << "::" << __FUNCTION__ << " - Error while rendering the window: " << error << Log::endl;

    glfwMakeContextCurrent(NULL);
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

} // end of namespace
