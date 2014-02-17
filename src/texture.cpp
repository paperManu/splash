#include "texture.h"
#include "timer.h"

#include <string>

using namespace std;
using namespace OIIO_NAMESPACE;

namespace Splash {

/*************/
Texture::Texture()
{
    _type = "texture";
    _timestamp = chrono::high_resolution_clock::now();
}

/*************/
Texture::Texture(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                 GLint border, GLenum format, GLenum type, const GLvoid* data)
{
    _type = "texture";
    _timestamp = chrono::high_resolution_clock::now();
    reset(target, level, internalFormat, width, height, border, format, type, data); 
}

/*************/
Texture::~Texture()
{
    SLog::log << Log::DEBUG << "Texture::~Texture - Destructor" << Log::endl;
    glDeleteTextures(1, &_glTex);
}

/*************/
Texture& Texture::operator=(ImagePtr& img)
{
    _img = img;
    return *this;
}

/*************/
void Texture::generateMipmap() const
{
    glBindTexture(GL_TEXTURE_2D, _glTex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/*************/
void Texture::reset(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid* data)
{
    if (width == 0 || height == 0)
    {
        SLog::log << Log::WARNING << "Texture::" << __FUNCTION__ << " - Texture size is null" << Log::endl;
        return;
    }

    if (_glTex == 0)
    {
        glGenTextures(1, &_glTex);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _glTex);

        if (internalFormat == GL_DEPTH_COMPONENT)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _glTex);
        glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    _spec.width = width;
    _spec.height = height;
    if (format == GL_RGB && type == GL_UNSIGNED_BYTE)
    {
        _spec.nchannels = 3;
        _spec.format = TypeDesc::UINT8;
        _spec.channelnames = {"R", "G", "B"};
    }
    else if (format == GL_RGBA && (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV))
    {
        _spec.nchannels = 4;
        _spec.format = TypeDesc::UINT8;
        _spec.channelnames = {"R", "G", "B", "A"};
    }
    else if (format == GL_R && type == GL_UNSIGNED_SHORT)
    {
        _spec.nchannels = 1;
        _spec.format = TypeDesc::UINT16;
        _spec.channelnames = {"R"};
    }

    _texTarget = target;
    _texLevel = level;
    _texInternalFormat = internalFormat;
    _texBorder = border;
    _texFormat = format;
    _texType = type;

    SLog::log << Log::DEBUG << "Texture::" << __FUNCTION__ << " - Reset the texture to size " << width << "x" << height << Log::endl;
}

/*************/
void Texture::resize(int width, int height)
{
    if (width != _spec.width && height != _spec.height)
        reset(_texTarget, _texLevel, _texInternalFormat, width, height, _texBorder, _texFormat, _texType, 0);
}

/*************/
bool Texture::linkTo(BaseObjectPtr obj)
{
    if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        ImagePtr img = dynamic_pointer_cast<Image>(obj);
        _img = img;
        return true;
    }

    return false;
}

/*************/
void Texture::update()
{
    // If _img is nullptr, this texture is not set from an Image
    if (_img.get() == nullptr)
        return;

    if (_img->getTimestamp() == _timestamp)
        return;
    _img->update();

    ImageSpec spec = _img->getSpec();
    vector<Value> srgb;
    _img->getAttribute("srgb", srgb);

    if (!(bool)glIsTexture(_glTex))
    {
        glGenTextures(1, &_glTex);
        return;
    }

    if (spec.width != _spec.width || spec.height != _spec.height || spec.nchannels != _spec.nchannels || spec.format != _spec.format)
    {
        glBindTexture(GL_TEXTURE_2D, _glTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (spec.nchannels == 4 && spec.format == "uint8")
        {
            SLog::log << Log::DEBUG << "Texture::" <<  __FUNCTION__ << " - Creating a new texture of type GL_UNSIGNED_BYTE, format GL_RGBA" << Log::endl;
            _img->lock();
            if (srgb[0].asInt() > 0)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, spec.width, spec.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, _img->data());
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, spec.width, spec.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, _img->data());
            _img->unlock();
        }
        else if (spec.nchannels == 1 && spec.format == "uint16")
        {
            SLog::log << Log::DEBUG << "Texture::" <<  __FUNCTION__ << " - Creating a new texture of type GL_UNSIGNED_SHORT, format GL_R" << Log::endl;
            _img->lock();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, spec.width, spec.height, 0, GL_RED, GL_UNSIGNED_SHORT, _img->data());
            _img->unlock();
        }
        else
        {
            SLog::log << Log::WARNING << "Texture::" <<  __FUNCTION__ << " - Texture format not supported" << Log::endl;
            return;
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        _spec = spec;
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, _glTex);
        _img->lock();
        if (spec.nchannels == 4 && spec.format == "uint8")
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, _img->data());
        else if (spec.nchannels == 1 && spec.format == "uint16")
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height, GL_RED, GL_UNSIGNED_SHORT, _img->data());
        _img->unlock();

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    _timestamp = _img->getTimestamp();
}

} // end of namespace
