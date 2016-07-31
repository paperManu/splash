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
 * @scene.h
 * The Scene class
 */

#ifndef SPLASH_SCENE_H
#define SPLASH_SCENE_H

#include <atomic>
#include <cstddef>
#include <future>
#include <list>
#include <vector>

#include "./config.h"

#if HAVE_GPHOTO
    #include "./colorcalibrator.h"
#endif
#include "./coretypes.h"
#include "./controller_gui.h"
#include "./basetypes.h"
#include "./controller.h"
#include "./factory.h"

namespace Splash {

class Scene;


/*************/
class Scene : public RootObject
{
    friend ControllerObject;
#if HAVE_GPHOTO
    friend ColorCalibrator;
#endif
    friend GuiControl;
    friend GuiGlobalView;
    friend GuiMedia;
    friend GuiNodeView;
    friend GuiWarp;
    friend GuiWidget;
    friend Gui;

    public:
        /**
         * Constructor
         */
        Scene(std::string name = "Splash", bool autoRun = true);

        /**
         * Destructor
         */
        ~Scene();

        /**
         * Add an object of the given type, with the given name
         */
        std::shared_ptr<BaseObject> add(std::string type, std::string name = std::string());

        /**
         * Add a fake object, keeping only its configuration between uses
         */
        void addGhost(std::string type, std::string name = std::string());

        /**
         * Get an attribute for the given object
         * Trie locally and to the World
         */
        Values getAttributeFromObject(std::string name, std::string attribute);

        /**
         * Get an attribute description
         * Try locally and to the World
         */
        Values getAttributeDescriptionFromObject(std::string name, std::string attribute);

        /**
         * Get the current configuration of the scene as a json object
         */
        Json::Value getConfigurationAsJson();

        /**
         * Get a glfw window sharing the same context as _mainWindow
         */
        std::shared_ptr<GlWindow> getNewSharedWindow(std::string name = std::string());

        /**
         * Get the list of objects by their type
         */
        Values getObjectsNameByType(std::string type);

        /**
         * Get the status of the scene, return true if all is well
         */
        bool getStatus() const {return _status;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Returns whether the scene is Master or not
         */
        bool isMaster() const {return _isMaster;}

        /**
         * Check wether the scene is running
         */
        bool isRunning() const {return _isRunning;}

        /**
         * Link / unlink an object to another, base on their types
         */
        bool link(std::string first, std::string second);
        bool link(std::shared_ptr<BaseObject> first, std::shared_ptr<BaseObject> second);
        void unlink(std::string first, std::string second);
        void unlink(std::shared_ptr<BaseObject> first, std::shared_ptr<BaseObject> second);

        /**
         * Link / unlink objects, at least one of them being a ghost
         */
        bool linkGhost(std::string first, std::string second);
        void unlinkGhost(std::string first, std::string second);

        /**
         * Remove an object
         */
        void remove(std::string name);

        /**
         * Render everything
         */
        void render();

        /**
         * Render the blending
         */
        void renderBlending();

        /**
         * Main loop for the scene
         */
        void run();

        /**
         * Set the Scene as the master one
         */
        void setAsMaster(std::string configFilePath = "");

        /**
         * Give a special behavior to the scene, making it the main window of the World
         */
        void setAsWorldScene();

        /**
         * Set a message to be sent to the world
         */
        void sendMessageToWorld(const std::string& message, const Values& value = {});
        Values sendMessageToWorldWithAnswer(const std::string& message, const Values& value = {}, const unsigned long long timeout = 0);

        /**
         * Wait for synchronization with texture upload
         * Has to be called from a GL context
         */
        void waitTextureUpload();

    protected:
        std::unique_ptr<Factory> _factory {nullptr};
        std::shared_ptr<GlWindow> _mainWindow;
        std::vector<int> _glVersion {0, 0};
        bool _isRunning {false};

        std::unordered_map<std::string, std::shared_ptr<BaseObject>> _ghostObjects;

        // Gui exists in master scene whatever the configuration
        std::shared_ptr<Gui> _gui;
        bool _guiLinkedToWindow {false};

        // Objects in charge of calibration
#if HAVE_GPHOTO
        std::shared_ptr<ColorCalibrator> _colorCalibrator;
#endif

        /**
         * Creates the blending map from the current calibration of the cameras
         */
        void computeBlendingMap(const std::string& mode = "once");
        void activateBlendingMap(bool once = true);
        void deactivateBlendingMap();

    private:
        static bool _isGlfwInitialized;

        std::shared_ptr<Scene> _self;
        bool _started {false};

        bool _isMaster {false}; //< Set to true if this is the master Scene of the current config
        bool _isInitialized {false};
        bool _status {false}; //< Set to true if an error occured during rendering
        int _swapInterval {1}; //< Global value for the swap interval, default for all windows

        // Joystick update loop and attributes
        std::future<void> _joystickUpdateFuture;
        std::mutex _joystickUpdateMutex;
        std::vector<float> _joystickAxes;
        std::vector<uint8_t> _joystickButtons;

        // Texture upload context
        std::future<void> _textureUploadFuture;
        std::condition_variable _textureUploadCondition;
        std::shared_ptr<GlWindow> _textureUploadWindow;
        std::atomic_bool _textureUploadDone {false};
        std::mutex _textureUploadMutex;
        GLsync _textureUploadFence, _cameraDrawnFence;
        
        // Vertex blending variables
        std::mutex _vertexBlendingMutex;
        std::condition_variable _vertexBlendingCondition;
        std::atomic_bool _vertexBlendingReceptionStatus {false};

        // NV Swap group specific
        GLuint _maxSwapGroups {0};
        GLuint _maxSwapBarriers {0};

        unsigned long _nextId {0};

        // Blending attributes
        bool _isBlendingComputed {false};
        bool _computeBlending {false};
        bool _computeBlendingOnce {false};
        unsigned int _blendingResolution {2048};
        std::shared_ptr<Texture_Image> _blendingTexture;
        std::shared_ptr<Image> _blendingMap;

        /**
         * Find which OpenGL version is available
         * Returns MAJOR and MINOR
         */
        std::vector<int> findGLVersion();

        /**
         * Set up the context and everything
         */
        void init(std::string name);

        /**
         * Set up the blending map
         */
        void initBlendingMap();

        /**
         * Joystick loop
         */
        void joystickUpdateLoop();

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
#ifdef HAVE_OSX
        static void glMsgCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);
#else
        static void glMsgCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, void*);
#endif

        /**
         * Texture update loop
         */
        void textureUploadRun();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();

        /**
         * Update the various inputs (mouse, keyboard...)
         */
        void updateInputs();
};

} // end of namespace

#endif // SPLASH_SCENE_H
