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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @texture.h
 * The Texture base class
 */

#ifndef SPLASH_TEXTURE_H
#define SPLASH_TEXTURE_H

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"

#include <chrono>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <OpenImageIO/imagebuf.h>

namespace oiio = OIIO_NAMESPACE;

namespace Splash {

class Texture : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Texture();
        Texture(RootObjectWeakPtr root);
        Texture(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* data);
        Texture(RootObjectWeakPtr root, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* data);

        /**
         * Destructor
         */
        ~Texture();

        /**
         * No copy constructor, but a move one
         */
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& t)
        {
            *this = std::move(t);
        }

        Texture& operator=(Texture&& t)
        {
            if (this != &t)
            {
                _glTex = t._glTex;
                _spec = t._spec;
                _pbos[0] = t._pbos[0];
                _pbos[1] = t._pbos[1];
                _pboReadIndex = t._pboReadIndex;

                _filtering = t._filtering;
                _texTarget = t._texTarget;
                _texLevel = t._texLevel;
                _texInternalFormat = t._texInternalFormat;
                _texBorder = t._texBorder;
                _texFormat = t._texFormat;
                _texType = t._texType;

                _img = t._img;
                _timestamp = t._timestamp;
            }
            return *this;
        }

        /**
         * Sets the specified buffer as the texture on the device
         */
        Texture& operator=(ImagePtr& img);

        /**
         * Bind / unbind this texture
         */
        void bind();
        void unbind();

        /**
         * Enable / disable filtering of the texture
         */
        void disableFiltering() {_filtering = false;}
        void enableFiltering() {_filtering = true;}

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
        std::map<std::string, Values> getShaderUniforms() const {return _shaderUniforms;}

        /**
         * Get spec of the texture
         */
        oiio::ImageSpec getSpec() const {return _spec;}

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);

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
        mutable std::mutex _mutex;

        GLuint _glTex = {0};
        oiio::ImageSpec _spec;
        GLuint _pbos[2];
        int _pboReadIndex {0};
        std::vector<unsigned int> _pboCopyThreadIds;

        // Store some texture parameters
        bool _resizable {true};
        bool _filtering {true};
        GLenum _texTarget, _texFormat, _texType;
        GLint _texLevel, _texInternalFormat, _texBorder;

        // And some temporary attributes
        GLint _activeTexture; // To which texture unit the texture is bound

        ImagePtr _img;
        std::chrono::high_resolution_clock::time_point _timestamp;

        // Parameters to send to the shader
        std::map<std::string, Values> _shaderUniforms;

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

typedef std::shared_ptr<Texture> TexturePtr;

} // end of namespace

#endif // SPLASH_TEXTURE_H
