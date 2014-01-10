#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

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

    _eye = vec3(1.0, 1.0, 5.0);
    _target = vec3(0.0, 0.0, 0.0);

    registerAttributes();
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

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    GLenum fboBuffers[_outTextures.size()];
    for (int i = 0; i < _outTextures.size(); ++i)
        fboBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    glDrawBuffers(_outTextures.size(), fboBuffers);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto obj : _objects)
    {
        obj->activate();
        obj->setViewProjectionMatrix(computeViewProjectionMatrix());
        obj->draw();
        obj->deactivate();
    }

    // We need to regenerate the mipmaps for all the output textures
    glActiveTexture(GL_TEXTURE0);
    for (auto t : _outTextures)
        t->generateMipmap();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    if (nbr < _outTextures.size())
    {
        for (int i = nbr; i < _outTextures.size(); ++i)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);

        _outTextures.resize(nbr);
    }
    else
    {
        for (int i = _outTextures.size(); i < nbr; ++i)
        {
            TexturePtr texture(new Texture);
            texture->reset(GL_TEXTURE_2D, 0, GL_RGB8, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            _outTextures.push_back(texture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture->getTexId(), 0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*************/
void Camera::setOutputSize(int width, int height)
{
    for (auto tex : _outTextures)
    {
        tex->reset(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
}

/*************/
mat4x4 Camera::computeViewProjectionMatrix()
{
    mat4x4 viewMatrix = lookAt(_eye, _target, glm::vec3(0.0, 0.0, 1.0));
    mat4x4 projMatrix = perspectiveFov(_fov, _width, _height, _near, _far);
    mat4x4 viewProjectionMatrix = viewMatrix * projMatrix;

    return viewProjectionMatrix;
}

/*************/
void Camera::registerAttributes()
{
    _attribFunctions["eye"] = AttributeFunctor([&](vector<float> args) {
        if (args.size() < 3)
            return;

        _eye = vec3(args[0], args[1], args[2]);
    });

    _attribFunctions["target"] = AttributeFunctor([&](vector<float> args) {
        if (args.size() < 3)
            return;

        _target = vec3(args[0], args[1], args[2]);
    });
}

} // end of namespace
