#include "window.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Splash {

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

    glfwMakeContextCurrent(NULL);

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

    glGetError();
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT);

    _screen->activate();
    _screen->setViewProjectionMatrix(glm::ortho(-1.f, 1.f, -1.f, 1.f));
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

} // end of namespace
