#include "window.h"

namespace Splash {

/*************/
Window::Window(GlWindowPtr w)
{
    if (w.get() == nullptr)
        return;

    _window = w;
    glfwMakeContextCurrent(_window->get());
    glfwShowWindow(w->get());
    glfwSwapInterval(0);

    glfwMakeContextCurrent(NULL);

    _screen.reset(new Object());
    GeometryPtr virtualScreen(new Geometry());
    _screen->addGeometry(virtualScreen);

    _isInitialized = true;
}

/*************/
Window::~Window()
{
}

/*************/
void Window::render()
{
    glfwMakeContextCurrent(_window->get());

    int w, h;
    glfwGetWindowSize(_window->get(), &w, &h);
    glViewport(0, 0, w, h);

    glClear(GL_COLOR_BUFFER_BIT);

    _screen->activate();

    glfwSwapBuffers(_window->get());

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

} // end of namespace
