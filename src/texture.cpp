#include "texture.h"

#include <string>

using namespace std;

namespace Splash {

/*************/
Texture::Texture()
{
}

/*************/
Texture::~Texture()
{
}

/*************/
template<typename DataType>
Texture& Texture::operator=(const ImageBuf& pImg)
{
    if (!pImg.initialized())
        return *this;
    if (pImg.localpixels() == NULL)
        return *this; // Pixels are not loaded in RAM, not yet supported

    ImageSpec spec = pImg.spec();

    if (spec.width != _spec.width || spec.height != _spec.width
        || spec.nchannels != _spec.nchannels || spec.format != _spec.format
        || !glIsTexture(_glTex))
    {
        glDeleteTextures(1, &_glTex);
        glGenTextures(1, &_glTex);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _glTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (spec.nchannels == 3 && spec.format == TypeDesc::UINT8)
        {
            gLog << string(__FUNCTION__) + string("Creating a new texture of type GL_UNSIGNED_BYTE, format GL_BGR");
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spec.width, spec.height, 0, GL_RGB, GL_UNSIGNED_BYTE, pImg.localpixels());
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        _spec = spec;
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _glTex);
        if (spec.nchannels == 3 && spec.format == TypeDesc::UINT8)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height, GL_RGB, GL_UNSIGNED_BYTE, pImg.localpixels());
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return *this;
}

/*************/
template<typename DataType>
ImageBuf Texture::getBuffer() const
{
}

/*************/
void Texture::reset(GLenum target, GLint pLevel, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid* data)
{
}

} // end of namespace
