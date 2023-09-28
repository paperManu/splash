#ifndef SPLASH_GLES_TEXTURE_IMAGE_H
#define SPLASH_GLES_TEXTURE_IMAGE_H

#include "graphics/texture_image.h"

namespace Splash {
    class GLESTexture_Image final: public Texture_Image 
    {
	public:
	    explicit GLESTexture_Image(RootObject* root): Texture_Image(root) { }

	    GLESTexture_Image(const GLESTexture_Image&) = delete;
	    GLESTexture_Image& operator=(const GLESTexture_Image&) = delete;
	    GLESTexture_Image(GLESTexture_Image&&) = delete;
	    GLESTexture_Image& operator=(GLESTexture_Image&&) = delete;

	    virtual std::unordered_map<std::string, InitTuple> getPixelFormatToInitTable() const final;
	    
	    virtual void bind() final;

	    virtual void unbind() final;

	    virtual void generateMipmap() const final;

	    virtual void getTextureImage(GLuint textureId, GLenum textureType, GLint level, GLenum format, GLenum type, GLsizei /*bufSize*/, void *pixels) const final;

	    virtual void getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const final;

	    virtual void getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const final;

	    virtual bool updatePbos(int width, int height, int bytes) final;

	    std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb);

	    void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const;

	    virtual void copyPixelsBetweenPBOs(int imageDataSize) const final;
	};
}

#endif
