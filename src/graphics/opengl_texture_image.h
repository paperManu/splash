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

	    virtual std::unordered_map<std::string, InitTuple> getPixelFormatToInitTable() const final;

	    virtual void bind() final;

	    void unbind() final;

	    virtual void generateMipmap() const final;

	    virtual void getTextureImage(GLuint textureId, GLenum /*textureType*/, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) const final;

	    virtual void getTextureLevelParameteriv(GLenum /*target*/, GLint level, GLenum pname, GLint* params) const final;

	    virtual void getTextureParameteriv(GLenum /*target*/, GLenum pname, GLint* params) const final;

	    virtual bool updatePbos(int width, int height, int bytes) final;

	    std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb);

	    void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const;

	    virtual void copyPixelsBetweenPBOs(int imageDataSize) const final;
	private:
	    GLubyte* _pbosPixels[2];
    };
}

#endif
