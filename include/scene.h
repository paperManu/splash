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

#ifndef SPLASH_SCENE_H
#define SPLASH_SCENE_H

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"

#include <atomic>
#include <cstddef>
#include <vector>
#include <json/reader.h>

namespace Splash {

class Scene;
typedef std::shared_ptr<Scene> ScenePtr;

/*************/
class Scene : public RootObject
{
    friend Gui;
    friend GlvGlobalView;
    friend GlvControl;

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
         * Get a glfw window sharing the same context as _mainWindow
         */
        GlWindowPtr getNewSharedWindow(std::string name = std::string(), bool gl2 = false);

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
         * Link / unlink an object to another, base on their types
         */
        bool link(std::string first, std::string second);
        bool link(BaseObjectPtr first, BaseObjectPtr second);
        bool unlink(std::string first, std::string second);
        bool unlink(BaseObjectPtr first, BaseObjectPtr second);

        /**
         * Link / unlink objects, at least one of them being a ghost
         */
        bool linkGhost(std::string first, std::string second);
        bool unlinkGhost(std::string first, std::string second);

        /**
         * Remove an object
         */
        void remove(std::string name);

        /**
         * Render everything
         */
        void render();

        /**
         * Set the Scene as the master one
         */
        void setAsMaster() {_isMaster = true;}

        /**
         * Give a special behavior to the scene, making it the main window of the World
         */
        void setAsWorldScene();

        /**
         * Set a message to be sent to the world
         */
        void sendMessageToWorld(const std::string message, const Values value = {});

        /**
         * Wait for synchronization with texture upload
         * Has to be called from a GL context
         */
        void waitTextureUpload();

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
        bool _started {false};
        std::thread _sceneLoop;
        std::recursive_mutex _configureMutex;

        bool _isMaster {false}; //< Set to true if this is the master Scene of the current config
        bool _isInitialized {false};
        bool _status {false}; //< Set to true if an error occured during rendering
        bool _isBlendComputed {false};
        int _swapInterval {1}; //< Global value for the swap interval, default for all windows

        // Texture upload context
        std::thread _textureUploadLoop;
        GlWindowPtr _textureUploadWindow;
        std::atomic_bool _textureUploadDone {false};
        std::mutex _textureUploadMutex;
        GLsync _textureUploadFence;

        // NV Swap group specific
        GLuint _maxSwapGroups {0};
        GLuint _maxSwapBarriers {0};

        unsigned long _nextId {0};

        // Blending map, used by all cameras (except the GUI camera)
        unsigned int _blendingResolution {2048};
        TexturePtr _blendingTexture;
        ImagePtr _blendingMap;

        // List of actions to do during the next render loop
        bool _doComputeBlending {false};
        bool _doSaveNow {false};

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
         * Callback for GL errors and warnings
         */
        static void glMsgCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);

        /**
         * Main loop for the scene
         */
        void run();

        /**
         * Texture update loop
         */
        void textureUploadRun();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Scene> ScenePtr;
typedef std::weak_ptr<Scene> SceneWeakPtr;

} // end of namespace

#endif // SPLASH_SCENE_H
