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
 * @scene.h
 * The Scene class
 */

#ifndef SCENE_H
#define SCENE_H

#include "config.h"

#include <vector>

#include "camera.h"
#include "geometry.h"
#include "object.h"
#include "texture.h"
#include "window.h"

namespace Splash {

class Scene {
    public:
        /**
         * Constructor
         */
        Scene();

        /**
         * Destructor
         */
        ~Scene();

    private:
        std::vector<ObjectPtr> _objects;
        std::vector<GeometryPtr> _geometries;
        std::vector<TexturePtr> _textures;

        std::vector<CameraPtr> _cameras;
        std::vector<WindowPtr> _windows;
};

} // end of namespace

#endif // SCENE_H
