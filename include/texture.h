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
        virtual ~Texture();

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
                _spec = t._spec;
                _timestamp = t._timestamp;
            }
            return *this;
        }

        /**
         * Bind / unbind this texture
         */
        virtual void bind() = 0;
        virtual void unbind() = 0;

        /**
         * Get the shader parameters related to this texture
         * Texture should be locked first
         */
        virtual std::unordered_map<std::string, Values> getShaderUniforms() const = 0;

        /**
         * Get spec of the texture
         */
        virtual oiio::ImageSpec getSpec() const = 0;

        /**
         * Try to link the given BaseObject to this
         */
        virtual bool linkTo(BaseObjectPtr obj);

        /**
         * Lock the texture for read / write operations
         */
        void lock() const {_mutex.lock();}

        /**
         * Unlock the texture for read / write operations
         */
        void unlock() const {_mutex.unlock();}

        /**
         * Update the texture according to the owned Image
         */
        virtual void update() = 0;

    protected:
        mutable std::mutex _mutex;
        oiio::ImageSpec _spec;

        // Store some texture parameters
        bool _resizable {true};
        bool _filtering {true};

        std::chrono::high_resolution_clock::time_point _timestamp;

    private:
        /**
         * As says its name
         */
        void init();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Texture> TexturePtr;

} // end of namespace

#endif // SPLASH_TEXTURE_H
