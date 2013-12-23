#include "texture.h"

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
Texture& Texture::operator=(const ImageBuf pImg)
{
}

/*************/
template<typename DataType>
ImageBuf Texture::getBuffer()
{
}

/*************/
void Texture::reset(GLenum target, GLint pLevel, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid* data);
{
}
