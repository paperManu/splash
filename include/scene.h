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

#include <cstddef>
#include <vector>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "coretypes.h"
#include "geometry.h"
#include "image.h"
#include "log.h"
#include "mesh.h"
#include "object.h"
#include "texture.h"
#include "threadpool.h"
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
         * Get the status of the scene, return true if all is well
         */
        bool getStatus() const {return _status;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Link an object to another, base on their types
         */
        bool link(std::string first, std::string second);
        bool link(BaseObjectPtr first, BaseObjectPtr second);

        /**
         * Render everything
         */
        bool render();

        /**
         * Set a parameter for an object, given its id
         */
        void setAttribute(std::string name, std::string attrib, std::vector<Value> args);

        /**
         * Set an object from its serialized form
         */
        void setFromSerializedObject(const std::string name, const SerializedObject& obj);

    private:
        bool _isInitialized {false};
        bool _status {false}; //< Set to true if an error occured during rendering
        GlWindowPtr _mainWindow;
        unsigned long _nextId {0};

        std::map<std::string, ObjectPtr> _objects;
        std::map<std::string, GeometryPtr> _geometries;
        std::map<std::string, MeshPtr> _meshes;
        std::map<std::string, ImagePtr> _images;
        std::map<std::string, TexturePtr> _textures;

        std::map<std::string, CameraPtr> _cameras;
        std::map<std::string, WindowPtr> _windows;

        ThreadPoolPtr _threadPool;

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

typedef std::shared_ptr<Scene> ScenePtr;

} // end of namespace

#endif // SCENE_H
