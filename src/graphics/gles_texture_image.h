#ifndef SPLASH_GLES_TEXTURE_IMAGE_H
#define SPLASH_GLES_TEXTURE_IMAGE_H

#include "graphics/texture_image.h"

namespace Splash {
    class GLESTexture_Image final: public Texture_Image 
    {
	public:
	    explicit GLESTexture_Image(RootObject* root): Texture_Image(root) 
	{ }

	    GLESTexture_Image(const GLESTexture_Image&) = delete;
	    GLESTexture_Image& operator=(const GLESTexture_Image&) = delete;
	    GLESTexture_Image(GLESTexture_Image&&) = delete;
	    GLESTexture_Image& operator=(GLESTexture_Image&&) = delete;

	    // Lists the supported combinations of internal formats, formats, and texture types: https://docs.gl/es3/glTexStorage2D
	    virtual void initFromPixelFormat(int width, int height) 
	    {

		// OpenGL ES doesn't support 16 bpc (bit per channel) RGBA textures, so we treat them as 8 bpc
		if (_pixelFormat == "RGBA" || _pixelFormat == "RGBA16")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_RGBA8;
		    _texFormat = GL_RGBA;

		    // OpenGL 4 vs ES 3.1: GL_UNSIGNED_INT_8_8_8_8_REV seems to be unavailable
		    // The docs say to use GL_RGBA8, GL_RGBA, and GL_UNSIGNED_BYTE for the internal format,
		    // texture format, and type respectively.
		    _texType = GL_UNSIGNED_BYTE;
		}
		else if (_pixelFormat == "sRGBA")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_SRGB8_ALPHA8;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_BYTE;
		}
		else if (_pixelFormat == "RGB")
		{
		    _spec = ImageBufferSpec(width, height, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
		    _texInternalFormat = GL_RGBA8;
		    _texFormat = GL_RGB;
		    _texType = GL_UNSIGNED_BYTE;
		}
		else if (_pixelFormat == "R16")
		{
		    _spec = ImageBufferSpec(width, height, 1, 16, ImageBufferSpec::Type::UINT16, "R");
		    _texInternalFormat = GL_R16;
		    _texFormat = GL_RED;
		    _texType = GL_UNSIGNED_SHORT;
		}
		else if (_pixelFormat == "YUYV" || _pixelFormat == "UYVY")
		{
		    _spec = ImageBufferSpec(width, height, 3, 16, ImageBufferSpec::Type::UINT8, _pixelFormat);
		    _texInternalFormat = GL_RG8;
		    _texFormat = GL_RG;
		    _texType = GL_UNSIGNED_SHORT;
		}
		else if (_pixelFormat == "D")
		{
		    // OpenGL ES supports only GL_DEPTH_COMPONENT32F for float values,
		    // even though we're using 24bit float value, this works fine.
		    _spec = ImageBufferSpec(width, height, 1, 24, ImageBufferSpec::Type::FLOAT, "R");
		    _texInternalFormat = GL_DEPTH_COMPONENT32F;
		    _texFormat = GL_DEPTH_COMPONENT;
		    _texType = GL_FLOAT;
		} else {
		    _spec.width = width;
		    _spec.height = height;

		    Log::get() << Log::WARNING << "GLESTexture_Image::" << __FUNCTION__ << " - The given pixel format (" << _pixelFormat << ") does not match any of the supported types. Will use default values." << Log::endl;
		}
	    }

	    virtual void bind() final
	    {
		glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
		glActiveTexture(_activeTexture);
		glBindTexture(_textureType, _glTex);
	    }

	    void unbind() final
	    {
#ifdef DEBUG
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
		_lastDrawnTimestamp = Timer::getTime();
	    }

	    virtual void generateMipmap() const final
	    {
		glBindTexture(GL_TEXTURE_2D, _glTex);
		glGenerateMipmap(GL_TEXTURE_2D);
	    }

	    virtual void getTextureImage(GLuint textureId, GLenum textureType, GLint level, GLenum format, GLenum type, GLsizei /*bufSize*/, void *pixels) const final 
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

	    virtual void getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const final 
	    {
		glGetTexLevelParameteriv(target, level, pname, params);
	    }

	    virtual void getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const final
	    {
		glGetTexParameteriv(target, pname, params);
	    }

	    virtual bool updatePbos(int width, int height, int bytes) final
	    {
		glDeleteBuffers(2, _pbos);

		// TODO: I think this is the most lenient option, might be worth it to see how others affect things
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

	    std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) 
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

	    void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const
	    {
		// Fill one of the PBOs right now
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
		auto pixels = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, imageDataSize, GL_MAP_WRITE_BIT);

		if (pixels != nullptr)
		    memcpy((void*)pixels, img->data(), imageDataSize);

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	    }


	    virtual void copyPixelsBetweenPBOs(int imageDataSize) const final 
	    {
		glBindBuffer(GL_COPY_READ_BUFFER, _pbos[0]);
		glBindBuffer(GL_COPY_WRITE_BUFFER, _pbos[1]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, imageDataSize);
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	    }


	};
}

#endif
