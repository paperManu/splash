#include "./graphics/gles_texture_image.h"

namespace Splash 
{

/*************/
std::unordered_map<std::string, Texture_Image::InitTuple> GLESTexture_Image::getPixelFormatToInitTable() const 
{
    // Lists the supported combinations of internal formats, formats, and texture types: https://docs.gl/es3/glTexStorage2D
    return {
	// OpenGL ES doesn't support 16 bpc (bit per channel) RGBA textures, so we treat them as 8 bpc
	// OpenGL 4 vs ES 3.1: GL_UNSIGNED_INT_8_8_8_8_REV seems to be unavailable
	// The docs say to use GL_RGBA8, GL_RGBA, and GL_UNSIGNED_BYTE for the internal format,
	// texture format, and type respectively.
	{"RGBA", { 4, 32, ImageBufferSpec::Type::UINT8, "RGBA", GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }}, 
	    {"RGBA16", { 4, 32, ImageBufferSpec::Type::UINT8, "RGBA", GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }}, 

	    {"sRGBA", { 4, 32, ImageBufferSpec::Type::UINT8, "RGBA", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE } },
	    {"RGB", { 3, 32, ImageBufferSpec::Type::UINT8, "RGB", GL_RGBA8, GL_RGB, GL_UNSIGNED_BYTE } },
	    {"R16", { 1, 16, ImageBufferSpec::Type::UINT16, "R", GL_R16, GL_RED, GL_UNSIGNED_SHORT } },

	    {"YUYV", { 3, 16, ImageBufferSpec::Type::UINT8, _pixelFormat, GL_RG8, GL_RG, GL_UNSIGNED_SHORT, }},
	    {"UYVY", { 3, 16, ImageBufferSpec::Type::UINT8, _pixelFormat, GL_RG8, GL_RG, GL_UNSIGNED_SHORT, }},

	    // OpenGL ES supports only GL_DEPTH_COMPONENT32F for float values,
	    // even though we're using 24bit float value, this works fine.
	    {"D", { 1, 24, ImageBufferSpec::Type::FLOAT, "R", GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT } } 
    };
}

/*************/
void GLESTexture_Image::bind() 
{
    glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
    glActiveTexture(_activeTexture);
    glBindTexture(_textureType, _glTex);
}

/*************/
void GLESTexture_Image::unbind() 
{
#ifdef DEBUG
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
    _lastDrawnTimestamp = Timer::getTime();
}

/*************/
void GLESTexture_Image::generateMipmap() const 
{
    glBindTexture(GL_TEXTURE_2D, _glTex);
    glGenerateMipmap(GL_TEXTURE_2D);
}

/*************/
void GLESTexture_Image::getTextureImage(GLuint textureId, GLenum textureType, GLint level, GLenum format, GLenum type, GLsizei /*bufSize*/, void *pixels) const 
{
    // Source: https://stackoverflow.com/a/53993894

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glBindTexture(textureType, textureId);

    // Probably won't work for cubemaps?
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureType, textureId, level);

    GLint width, height;
    getTextureLevelParameteriv(textureType, level, GL_TEXTURE_WIDTH, &width);
    getTextureLevelParameteriv(textureType, level, GL_TEXTURE_HEIGHT, &height);

    glReadPixels(0, 0, width, height, format, type, pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}

/*************/
void GLESTexture_Image::getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const 
{
    glGetTexLevelParameteriv(target, level, pname, params);
}

/*************/
void GLESTexture_Image::getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const 
{
    glGetTexParameteriv(target, pname, params);
}

/*************/
bool GLESTexture_Image::updatePbos(int width, int height, int bytes) 
{
    glDeleteBuffers(2, _pbos);

    // I think this is the most lenient option, might be worth it to see how others affect things
    constexpr auto bufferUsageFlags = GL_STATIC_DRAW;
    auto imageDataSize = width * height * bytes;

    glGenBuffers(2, _pbos);

    constexpr GLenum bufferType = GL_PIXEL_UNPACK_BUFFER;

    glBindBuffer(bufferType, _pbos[0]);
    glBufferData(bufferType, imageDataSize, 0, bufferUsageFlags);

    glBindBuffer(bufferType, _pbos[1]);
    glBufferData(bufferType, imageDataSize, 0, bufferUsageFlags);

    glBindBuffer(bufferType, 0);

    return true;
}

/*************/
std::optional<std::pair<GLenum, GLenum>> GLESTexture_Image::updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) 
{
    GLenum internalFormat;
    GLenum dataFormat = GL_UNSIGNED_BYTE;

    if (spec.channels == 4 && spec.type == ImageBufferSpec::Type::UINT8)
    {
	dataFormat = GL_UNSIGNED_BYTE;
	_texFormat = GL_RGBA; // Not sure if we're supposed to modify data members here..
	if (srgb[0].as<bool>())
	    internalFormat = GL_SRGB8_ALPHA8;
	else
	    internalFormat = GL_RGBA;
    }
    else if (spec.channels == 3 && spec.type == ImageBufferSpec::Type::UINT8)
    {
	dataFormat = GL_UNSIGNED_BYTE;
	if (srgb[0].as<bool>())
	    internalFormat = GL_SRGB8_ALPHA8;
	else
	    internalFormat = GL_RGBA8;
    }
    else if (spec.channels == 2 && spec.type == ImageBufferSpec::Type::UINT8)
    {
	_texFormat = GL_RG;
	dataFormat = GL_UNSIGNED_BYTE;
	internalFormat = GL_RG8;
    }
    else if (spec.channels == 1 && spec.type == ImageBufferSpec::Type::UINT16)
    {
	dataFormat = GL_UNSIGNED_SHORT;
	internalFormat = GL_R16;
    }
    else
    {
	Log::get() << Log::WARNING << "GLESTexture_Image::" << __FUNCTION__ << " - Unknown uncompressed format" << Log::endl;
	return {};
    }

    return {{ internalFormat, dataFormat }};
}

/*************/
void GLESTexture_Image::readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const
{
    // Fill one of the PBOs right now
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
    auto pixels = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, imageDataSize, GL_MAP_WRITE_BIT);

    if (pixels != nullptr)
	memcpy((void*)pixels, img->data(), imageDataSize);

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/*************/
void GLESTexture_Image::copyPixelsBetweenPBOs(int imageDataSize) const  
{
    glBindBuffer(GL_COPY_READ_BUFFER, _pbos[0]);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _pbos[1]);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, imageDataSize);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

}
