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
 * @camera.h
 * The Camera base class
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <config.h>

#include <memory>
#include <vector>

#include "object.h"
#include "texture.h"

namespace Splash {

class Camera
{
    public:
        /**
         * Constructor
         */
        Camera();

        /**
         * Destructor
         */
        ~Camera();

        /**
         * Get pointers to this camera textures
         */
        std::vector<TexturePtr> getTextures() const {return _outTextures;}

        /**
         * Render this camera into its textures
         */
        void render();

        /**
         * Set the resolution of this camera
         */
        void setSize(int width, int height);

    private:
        GLuint _fbo;
        std::vector<TexturePtr> _outTextures;
        std::vector<ObjectPtr> _objects;
};

typedef std::shared_ptr<Camera> CameraPtr;

} // end of namespace

#endif // CAMERA_H
