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
#include "coretypes.h"

#include <cstddef>
#include <vector>
#include <GLFW/glfw3.h>
#include <json/reader.h>

#include "camera.h"
#include "geometry.h"
#include "gui.h"
#include "image.h"
#include "link.h"
#include "log.h"
#include "mesh.h"
#include "object.h"
#include "texture.h"
#include "threadpool.h"
#include "window.h"

namespace Splash {

class Scene;
typedef std::shared_ptr<Scene> ScenePtr;

/*************/
class Scene : public RootObject
{
    friend Gui;
    friend GlvGlobalView;

    public:
        /**
         * Constructor
         */
        Scene(std::string name = "Splash");

        /**
         * Destructor
         */
        ~Scene();

        /**
         * Add an object of the given type, with the given name
         */
        BaseObjectPtr add(std::string type, std::string name = std::string());

        /**
         * Add a fake object, keeping only its configuration between uses
         */
        void addGhost(std::string type, std::string name = std::string());

        /**
         * Get the current configuration of the scene as a json object
         */
        Json::Value getConfigurationAsJson();

        /**
         * Get the status of the scene, return true if all is well
         */
        bool getStatus() const {return _status;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Check wether the scene is running
         */
        bool isRunning() const {return _isRunning;}

        /**
         * Link an object to another, base on their types
         */
        bool link(std::string first, std::string second);
        bool link(BaseObjectPtr first, BaseObjectPtr second);

        /**
         * Link objects, at least one of them being a ghost
         */
        bool linkGhost(std::string first, std::string second);

        /**
         * Render everything
         */
        void render();

        /**
         * Main loop for the scene
         */
        void run();

        /**
         * Give a special behavior to the scene, making it the main window of the World
         */
        void setAsWorldScene();

        /**
         * Set a message to be sent to the world
         */
        void setMessage(const std::string message, const std::vector<Value> value = {});

    protected:
        GlWindowPtr _mainWindow;
        bool _isRunning {false};

        std::map<std::string, BaseObjectPtr> _ghostObjects;

        /**
         * Creates the blending map from the current calibration of the cameras
         */
        void computeBlendingMap();

    private:
        ScenePtr _self;
        std::shared_ptr<Link> _link;
        bool _started {false};
        std::thread _sceneLoop;

        bool _isInitialized {false};
        bool _status {false}; //< Set to true if an error occured during rendering
        bool _isBlendComputed {false};
        unsigned long _nextId {0};

        // Blending map, used by all cameras (except the GUI camera)
        TexturePtr _blendingTexture;
        ImagePtr _blendingMap;

        // List of actions to do during the next render loop
        bool _doComputeBlending {false};
        bool _doSaveNow {false};

        /**
         * Get a glfw window sharing the same context as _mainWindow
         */
        GlWindowPtr getNewSharedWindow(std::string name = std::string(), bool gl2 = false);

        /**
         * Set up the context and everything
         */
        void init(std::string name);

        /**
         * Set up the blending map
         */
        void initBlendingMap();

        /**
         * Get the next available id
         */
        unsigned long getId() {return ++_nextId;}

        /**
         * Callback for GLFW errors
         */
        static void glfwErrorCallback(int code, const char* msg);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Scene> ScenePtr;
typedef std::weak_ptr<Scene> SceneWeakPtr;

} // end of namespace

#endif // SCENE_H
