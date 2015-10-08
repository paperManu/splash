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
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <OpenImageIO/imagebuf.h>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "image.h"
#include "texture.h"

namespace oiio = OIIO_NAMESPACE;

namespace Splash {

class Texture_Image : public Texture
{
    public:
        /**
         * Constructor
         */
        Texture_Image();
        Texture_Image(RootObjectWeakPtr root);
        Texture_Image(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* data);
        Texture_Image(RootObjectWeakPtr root, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* data);

        /**
         * Destructor
         */
        ~Texture_Image();

        /**
         * No copy constructor, but a move one
         */
        Texture_Image(const Texture_Image&) = delete;
        Texture_Image& operator=(const Texture_Image&) = delete;

        /**
         * Sets the specified buffer as the texture on the device
         */
        Texture_Image& operator=(const std::shared_ptr<Image>& img);

        /**
         * Bind / unbind this texture
         */
        void bind();
        void unbind();

        /**
         * Flush the PBO copy which may still be happening. Do this before
         * closing the current context!
         */
        void flushPbo();

        /**
         * Generate the mipmaps for the texture
         */
        void generateMipmap() const;

        /**
         * Get the id of the gl texture
         */
        GLuint getTexId() const {return _glTex;}

        /**
         * Get the shader parameters related to this texture
         * Texture should be locked first
         */
        std::unordered_map<std::string, Values> getShaderUniforms() const {return _shaderUniforms;}

        /**
         * Get spec of the texture
         */
        oiio::ImageSpec getSpec() const {return _spec;}

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(std::shared_ptr<BaseObject> obj);

        /**
         * Lock the texture for read / write operations
         */
        void lock() const {_mutex.lock();}

        /**
         * Read the texture and returns an Image
         */
        ImagePtr read();

        /**
         * Set the buffer size / type / internal format
         * See glTexImage2D for information about parameters
         */
        void reset(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                   GLint border, GLenum format, GLenum type, const GLvoid* data);

        /**
         * Modify the size of the texture
         */
        void resize(int width, int height);

        /**
         * Unlock the texture for read / write operations
         */
        void unlock() const {_mutex.unlock();}

        /**
         * Update the texture according to the owned Image
         */
        void update();

    private:
        GLuint _glTex {0};
        GLuint _pbos[2];
        int _pboReadIndex {0};
        std::vector<unsigned int> _pboCopyThreadIds;

        // Store some texture parameters
        bool _filtering {true};
        GLenum _texTarget, _texFormat, _texType;
        GLint _texLevel, _texInternalFormat, _texBorder;

        // And some temporary attributes
        GLint _activeTexture; // Texture unit to which the texture is bound

        std::weak_ptr<Image> _img;

        // Parameters to send to the shader
        std::unordered_map<std::string, Values> _shaderUniforms;

        /**
         * As says its name
         */
        void init();

        /**
         * Update the pbos according to the parameters
         */
        void updatePbos(int width, int height, int bytes);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Texture_Image> Texture_ImagePtr;

} // end of namespace

#endif // SPLASH_TEXTURE_H
