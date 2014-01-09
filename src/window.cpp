#include "window.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

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

} // end of namespace
