#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

namespace Splash {

/*************/
Camera::Camera(GlWindowPtr w)
{
    _type = "camera";

    if (w.get() == nullptr)
        return;

    _window = w;
    glfwMakeContextCurrent(_window->get());

    glGetError();
    glGenFramebuffers(1, &_fbo);
    setOutputNbr(1);

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    GLenum _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        SLog::log << Log::WARNING << __FUNCTION__ << " - Error while initializing framebuffer object" << Log::endl;
    else
        SLog::log << Log::MESSAGE << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLenum error = glGetError();
    if (error)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Error while binding framebuffer" << Log::endl;
        _isInitialized = false;
    }
    else
        _isInitialized = true;

    glfwMakeContextCurrent(NULL);
}

/*************/
Camera::~Camera()
{
}

/*************/
void Camera::addObject(ObjectPtr& obj)
{
    _objects.push_back(obj);
}

/*************/
void Camera::render()
{
    glfwMakeContextCurrent(_window->get());

    if (_outTextures.size() < 1)
        return;

    glGetError();
    ImageSpec spec = _outTextures[0]->getSpec();
    glViewport(0, 0, spec.width, spec.height);

    for (auto obj : _objects)
    {
        obj->activate();
        obj->setViewProjectionMatrix(glm::ortho(-1.f, 1.f, -1.f, 1.f));
        obj->draw();
        obj->deactivate();
    }

    GLenum error = glGetError();
    if (error)
        SLog::log << Log::WARNING << _type << "::" << __FUNCTION__ << " - Error while rendering the camera: " << error << Log::endl;

    glfwMakeContextCurrent(NULL);
}

/*************/
void Camera::setOutputNbr(int nbr)
{
    if (nbr < 1 || nbr == _outTextures.size())
        return;

    if (nbr < _outTextures.size())
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        for (int i = nbr; i < _outTextures.size(); ++i)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        _outTextures.resize(nbr);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        for (int i = _outTextures.size(); i < nbr; ++i)
        {
            TexturePtr texture(new Texture);
            texture->reset(GL_TEXTURE_2D, 0, GL_RGB8, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            _outTextures.push_back(texture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture->getTexId(), 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

/*************/
void Camera::setOutputSize(int width, int height)
{
    for (auto tex : _outTextures)
    {
        tex->reset(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
}

} // end of namespace
