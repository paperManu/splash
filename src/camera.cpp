#include "camera.h"

using namespace std;

namespace Splash {

/*************/
Camera::Camera()
{
    glGenFramebuffers(1, &_fbo);
    setOutputNbr(1);

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        gLog(string(__FUNCTION__) + string(" - Error while initializing framebuffer object"), Log::WARNING);
    else
        gLog(string(__FUNCTION__) + string(" - Framebuffer object successfully initialized"), Log::DEBUG);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*************/
Camera::~Camera()
{
}

/*************/
void Camera::render()
{
    if (_outTextures.size() < 1)
        return;
          
    ImageSpec spec = _outTextures[0]->getSpec();
    glViewport(0, 0, spec.width, spec.height);

    for (auto obj : _objects)
    {
    }
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
