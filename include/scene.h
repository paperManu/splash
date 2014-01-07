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

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#define SPLASH_GL_CONTEXT_VERSION_MAJOR 3
#define SPLASH_GL_CONTEXT_VERSION_MINOR 2
#define SPLASH_GL_DEBUG false

#include "config.h"

#include <cstddef>
#include <vector>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "coretypes.h"
#include "geometry.h"
#include "log.h"
#include "mesh.h"
#include "object.h"
#include "texture.h"
#include "window.h"

namespace Splash {

/*************/
class Scene
{
    public:
        /**
         * Constructor
         */
        Scene();

        /**
         * Destructor
         */
        ~Scene();

        /**
         * Add an object of the given type, with the given name
         */
        BaseObjectPtr add(std::string type, std::string name = std::string());

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Link an object to another, base on their types
         */
        bool link(BaseObjectPtr first, BaseObjectPtr second);

        /**
         * Render everything
         */
        void render();

    private:
        bool _isInitialized {false};
        GlWindowPtr _mainWindow;
        unsigned long _nextId {0};

        std::map<std::string, ObjectPtr> _objects;
        std::map<std::string, GeometryPtr> _geometries;
        std::map<std::string, MeshPtr> _meshes;
        std::map<std::string, TexturePtr> _textures;

        std::map<std::string, CameraPtr> _cameras;
        std::map<std::string, WindowPtr> _windows;

        /**
         * Get a glfw window sharing the same context as _mainWindow
         */
        GlWindowPtr getNewSharedWindow();

        /**
         * Set up the context and everything
         */
        void init();

        /**
         * Get the next available id
         */
        unsigned long getId() {return ++_nextId;}

        /**
         * Callback for GLFW errors
         */
        static void glfwErrorCallback(int code, const char* msg);
};

} // end of namespace

#endif // SCENE_H
