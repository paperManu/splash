/*
 * Copyright (C) 2023 Tarek Yasser
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @gles_texture_image.h
 * Contains OpenGL ES 3.x specific functionality for textures.
 */

#ifndef SPLASH_GLES_TEXTURE_IMAGE_H
#define SPLASH_GLES_TEXTURE_IMAGE_H

#include "./graphics/texture_image.h"

namespace Splash 
{

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

	virtual std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb);

	virtual void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const;

	virtual void copyPixelsBetweenPBOs(int imageDataSize) const final;
};

}

#endif
