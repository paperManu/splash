#ifndef SPLASH_OPENGL_TEXTURE_IMAGE_H
#define SPLASH_OPENGL_TEXTURE_IMAGE_H

#include "./graphics/texture_image.h"

namespace Splash 
{
    class OpenGLTexture_Image final: public Texture_Image 
    {
	public:
	    explicit OpenGLTexture_Image(RootObject* root): Texture_Image(root) 
	{ }

	    OpenGLTexture_Image(const OpenGLTexture_Image&) = delete;
	    OpenGLTexture_Image& operator=(const OpenGLTexture_Image&) = delete;
	    OpenGLTexture_Image(OpenGLTexture_Image&&) = delete;
	    OpenGLTexture_Image& operator=(OpenGLTexture_Image&&) = delete;

	    virtual void initFromPixelFormat(int width, int height) 
	    {
		// Can probably be boiled down to a map of:
		// 	pixelFormat -> { numChannels, bitPerChannel, channelType, formatName, texInternalFormat, texFormat, texType }
		// But that's probably asking for trouble, let's keep it simple.
		if (_pixelFormat == "RGBA")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_RGBA8;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
		}
		else if (_pixelFormat == "sRGBA")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_SRGB8_ALPHA8;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
		}
		else if (_pixelFormat == "RGBA16")
		{
		    _spec = ImageBufferSpec(width, height, 4, 64, ImageBufferSpec::Type::UINT16, "RGBA");
		    _texInternalFormat = GL_RGBA16;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_SHORT;
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
		    _spec = ImageBufferSpec(width, height, 1, 24, ImageBufferSpec::Type::FLOAT, "R");
		    _texInternalFormat = GL_DEPTH_COMPONENT24;
		    _texFormat = GL_DEPTH_COMPONENT;
		    _texType = GL_FLOAT;
		} else {
		    _spec.width = width;
		    _spec.height = height;

		    Log::get() << Log::WARNING << "OpenGLTexture_Image::" << __FUNCTION__ << " - The given pixel format (" << _pixelFormat << ") does not match any of the supported types. Will use default values." << Log::endl;
		}
	    }

	    virtual void bind() final
	    {
		glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
		_activeTexture = _activeTexture - GL_TEXTURE0;
		glBindTextureUnit(_activeTexture, _glTex);
	    }

	    void unbind() final
	    {
#ifdef DEBUG
		glBindTextureUnit(_activeTexture, 0);
#endif
		_lastDrawnTimestamp = Timer::getTime();
	    }

	    virtual void generateMipmap() const final
	    {
		glGenerateTextureMipmap(_glTex);
	    }

	    virtual void getTextureImage(GLuint textureId, GLenum /*textureType*/, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) const final 
	    {
		glGetTextureImage(textureId, level, format, type, bufSize, pixels);
	    }

	    virtual void getTextureLevelParameteriv(GLenum /*target*/, GLint level, GLenum pname, GLint* params) const final {
		glGetTextureLevelParameteriv(_glTex, level, pname, params);
	    }

	    virtual void getTextureParameteriv(GLenum /*target*/, GLenum pname, GLint* params) const final
	    {
		glGetTextureParameteriv(_glTex, pname, params);
	    }

	    virtual bool updatePbos(int width, int height, int bytes) final
	    {
		glDeleteBuffers(2, _pbos);
		auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		auto imageDataSize = width * height * bytes;

		glCreateBuffers(2, _pbos);
		glNamedBufferStorage(_pbos[0], imageDataSize, 0, flags);
		glNamedBufferStorage(_pbos[1], imageDataSize, 0, flags);

		_pbosPixels[0] = (GLubyte*)glMapNamedBufferRange(_pbos[0], 0, imageDataSize, flags);
		_pbosPixels[1] = (GLubyte*)glMapNamedBufferRange(_pbos[1], 0, imageDataSize, flags);

		if (!_pbosPixels[0] || !_pbosPixels[1])
		{
		    Log::get() << Log::ERROR << "OpenGLTexture_Image::" << __FUNCTION__ << " - Unable to initialize upload PBOs" << Log::endl;
		    return false;
		}

		return true;
	    }

	    std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) 
	    {
		GLenum internalFormat;
		GLenum dataFormat = GL_UNSIGNED_BYTE;

		if (spec.channels == 4 && spec.type == ImageBufferSpec::Type::UINT8)
		{
		    dataFormat = GL_UNSIGNED_INT_8_8_8_8_REV;
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
		    Log::get() << Log::WARNING << "OpenGLTexture_Image::" << __FUNCTION__ << " - Unknown uncompressed format" << Log::endl;
		    return {};
		}

		return {{ internalFormat, dataFormat }};
	    }

	    void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const
	    {

		// Given an OpenGL PBO ID, we need to figure out which pixel buffer we need to read into.
		// This is done under the assumption that we have only 2 PBOs.
		int bufferIndex = 0;
		if(pboId == _pbos[0]) {
		    bufferIndex = 0;
		} else {
		    bufferIndex = 1;
		}
		
		// Fill the next PBO with the image pixels
		auto pixels = _pbosPixels[bufferIndex];
		if (pixels != nullptr)
		    memcpy(pixels, img->data(), imageDataSize);
	    }
	    virtual void copyPixelsBetweenPBOs(int imageDataSize) const final 
	    {
		glCopyNamedBufferSubData(_pbos[0], _pbos[1], 0, 0, imageDataSize);

	    }


	private:
	    GLubyte* _pbosPixels[2];
    };
}

#endif
