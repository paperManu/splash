/*
 * Copyright (C) 2013 Emmanuel Durand
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
 * @texture.h
 * The Texture_Image base class
 */

#ifndef SPLASH_TEXTURE_IMAGE_H
#define SPLASH_TEXTURE_IMAGE_H

#include <chrono>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "config.h"

#include "basetypes.h"
#include "coretypes.h"
#include "image.h"
#include "texture.h"

namespace Splash
{

class Texture_Image : public Texture
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     * \param target Texture target
     * \param level Mipmap level
     * \param internalFormat Texture internal format
     * \param width Width
     * \param height Height
     * \param border Texture border behavior
     * \param format Texture format
     * \param type Texture type
     * \param data Pointer to data to use to initialize the texture
     */
    Texture_Image(std::weak_ptr<RootObject> root);
    Texture_Image(std::weak_ptr<RootObject> root,
        GLenum target,
        GLint level,
        GLint internalFormat,
        GLsizei width,
        GLsizei height,
        GLint border,
        GLenum format,
        GLenum type,
        const GLvoid* data);

    /**
     * \brief Destructor
     */
    ~Texture_Image();

    /**
     * No copy constructor, but a move one
     */
    Texture_Image(const Texture_Image&) = delete;
    Texture_Image& operator=(const Texture_Image&) = delete;

    /**
     * \brief Sets the specified buffer as the texture on the device
     * \param img Image to set the texture from
     */
    Texture_Image& operator=(const std::shared_ptr<Image>& img);

    /**
     * \brief Bind this texture
     */
    void bind();

    /**
     * \brief Unbind this texture
     */
    void unbind();

    /**
     * \brief Flush the PBO copy which may still be happening. Do this before closing the current context!
     */
    void flushPbo();

    /**
     * \brief Generate the mipmaps for the texture
     */
    void generateMipmap() const;

    /**
     * \brief Get the id of the gl texture
     * \return Return the texture id
     */
    GLuint getTexId() const { return _glTex; }

    /**
     * \brief Get the shader parameters related to this texture. Texture should be locked first.
     * \return Return the shader uniforms
     */
    std::unordered_map<std::string, Values> getShaderUniforms() const { return _shaderUniforms; }

    /**
     * \brief Get spec of the texture
     * \return Return the spec
     */
    ImageBufferSpec getSpec() const { return _spec; }

    /**
     * \brief Try to link the given BaseObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(std::shared_ptr<BaseObject> obj);

    /**
     * \brief Lock the texture for read / write operations
     */
    void lock() const { _mutex.lock(); }

    /**
     * \brief Read the texture and returns an Image
     * \return Return the image
     */
    std::shared_ptr<Image> read();

    /**
     * \brief Set the buffer size / type / internal format
     * \param target Texture target
     * \param level Mipmap level
     * \param internalFormat Texture internal format
     * \param width Width
     * \param height Height
     * \param border Texture border behavior
     * \param format Texture format
     * \param type Texture type
     * \param data Pointer to data to use to initialize the texture
     */
    void reset(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);

    /**
     * \brief Modify the size of the texture
     * \param width Width
     * \param height Height
     */
    void resize(int width, int height);

    /**
     * \brief Enable / disable clamp to edge
     * \param active If true, enables clamping
     */
    void setClampToEdge(bool active) { _glTextureWrap = active ? GL_CLAMP_TO_EDGE : GL_REPEAT; }

    /**
     * \brief Unlock the texture for read / write operations
     */
    void unlock() const { _mutex.unlock(); }

    /**
     * \brief Update the texture according to the owned Image
     */
    void update();

  private:
    GLint _glVersionMajor{0};
    GLint _glVersionMinor{0};

    GLuint _glTex{0};
    GLuint _pbos[2];
    int _pboReadIndex{0};
    std::vector<unsigned int> _pboCopyThreadIds;

    // Store some texture parameters
    bool _filtering{false};
    GLenum _texTarget{GL_TEXTURE_2D}, _texFormat{GL_RGB}, _texType{GL_UNSIGNED_BYTE};
    GLint _texLevel{0}, _texInternalFormat{GL_CLAMP_TO_EDGE}, _texBorder{0};
    GLint _glTextureWrap{GL_REPEAT};

    // And some temporary attributes
    GLint _activeTexture{0}; // Texture unit to which the texture is bound

    std::weak_ptr<Image> _img;

    // Parameters to send to the shader
    std::unordered_map<std::string, Values> _shaderUniforms;

    /**
     * \brief Initialization
     */
    void init();

    /**
     * \brief Get GL channel order according to spec.format
     * \param spec Specification
     * \return Return the GL channel order (GL_RGBA, GL_BGRA, ...)
     */
    GLenum getChannelOrder(const ImageBufferSpec& spec);

    /**
     * \brief Update the pbos according to the parameters
     * \param width Width
     * \param height Height
     * \param bytes Bytes per pixel
     */
    void updatePbos(int width, int height, int bytes);

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_TEXTURE_H
