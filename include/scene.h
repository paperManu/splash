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
#include "./basetypes.h"
#include "./controller.h"
#include "./controller_gui.h"
#include "./coretypes.h"
#include "./factory.h"

namespace Splash
{

class Scene;

/*************/
//! Scene class, which does the rendering on a given GPU
class Scene : public RootObject
{
    friend ControllerObject;
    friend UserInput;
#if HAVE_GPHOTO
    friend ColorCalibrator;
#endif
    friend GuiControl;
    friend GuiGlobalView;
    friend GuiMedia;
    friend GuiMeshes;
    friend GuiNodeView;
    friend GuiWarp;
    friend GuiWidget;

  public:
    /**
     * \brief Constructor
     * \param name Scene name
     * \param autoRun If true, the Scene will start without waiting for a start message from the World
     */
    Scene(std::string name = "Splash");

    /**
     * \brief Destructor
     */
    ~Scene();

    /**
     * \brief Add an object of the given type, with the given name
     * \param type Object type
     * \param name Object name
     * \return Return a shared pointer to the created object
     */
    std::shared_ptr<BaseObject> add(std::string type, std::string name = std::string());

    /**
     * \brief Add an object ghosting one in another Scene. Used in master Scene for controlling purposes
     * \param type Object type
     * \param name Object name
     */
    void addGhost(std::string type, std::string name = std::string());

    /**
     * \brief Get an attribute for the given object. Try locally and to the World
     * \param name Object name
     * \param attribute Attribute
     * \return Return the attribute value
     */
    Values getAttributeFromObject(std::string name, std::string attribute);

    /**
     * \brief Get an attribute description. Try locally and to the World
     * \param name Object name
     * \param attribute Attribute
     * \return Return the attribute description
     */
    Values getAttributeDescriptionFromObject(std::string name, std::string attribute);

    /**
     * \brief Get the current configuration of the scene as a json object
     * \return Return a Json object holding the configuration
     */
    Json::Value getConfigurationAsJson();

    /**
     * \brief Get a glfw window sharing the same context as _mainWindow
     * \param name Window name
     * \return Return a shared pointer to the new window
     */
    std::shared_ptr<GlWindow> getNewSharedWindow(std::string name = std::string());

    /**
     * \brief Get the list of objects by their type
     * \param type Object type
     * \return Return the list of objects of the given type
     */
    Values getObjectsNameByType(std::string type);

    /**
     * \brief Get the status of the scene
     * \return Return true if all is well
     */
    bool getStatus() const { return _status; }

    /**
     * \brief Check whether it is initialized
     * \return Return true if the Scene is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     * \brief Ask whether the scene is Master or not
     * \return Return true if the Scene is master
     */
    bool isMaster() const { return _isMaster; }

    /**
     * \brief Check wether the scene is running
     * \return Return true if the scene runs
     */
    bool isRunning() const { return _isRunning; }

    /**
     * \brief Link an object to another, base on their types
     * \param first Child object
     * \param second Parent object
     * \return Return true if the linking succeeded
     */
    bool link(std::string first, std::string second);
    bool link(std::shared_ptr<BaseObject> first, std::shared_ptr<BaseObject> second);

    /**
     * \brief Unlink two objects. This always succeeds
     * \param first Child object
     * \param second Parent object
     */
    void unlink(std::string first, std::string second);
    void unlink(std::shared_ptr<BaseObject> first, std::shared_ptr<BaseObject> second);

    /**
     * \brief Link objects, one of them being a ghost
     * \param first Child object
     * \param second Parent object
     * \return Return true if the linking succeeded
     */
    bool linkGhost(std::string first, std::string second);

    /**
     * \brief Unlink two objects, one of them being a ghost
     * \param first Child object
     * \param second Parent object
     */
    void unlinkGhost(std::string first, std::string second);

    /**
     * \brief Remove an object
     * \param name Object name
     */
    void remove(std::string name);

    /**
     * \brief Render everything
     */
    void render();

    /**
     * \brief Main loop for the scene
     */
    void run();

    /**
     * \brief Set the Scene as the master one
     * \param configFilePath File path for the loaded configuration
     */
    void setAsMaster(std::string configFilePath = "");

    /**
     * \brief Give a special behavior to the scene, making it the main window of the World
     */
    void setAsWorldScene();

    /**
     * \brief Set a message to be sent to the world
     * \param message Message type to send, which should correspond to a World attribute
     * \param value Message content
     */
    void sendMessageToWorld(const std::string& message, const Values& value = {});

    /**
     * \brief Set a message to be sent to the world, and wait for the World to send an answer
     * \param message Message type to send, which should correspond to a World attribute
     * \param value Message content
     * \param timeout Timeout in microseconds
     * \return Return the answer from the World
     */
    Values sendMessageToWorldWithAnswer(const std::string& message, const Values& value = {}, const unsigned long long timeout = 0);

    /**
     * \brief Wait for synchronization with texture upload. This must to be called from a GL context
     */
    void waitTextureUpload();

  protected:
    std::unique_ptr<Factory> _factory{nullptr};
    std::shared_ptr<GlWindow> _mainWindow;
    std::vector<int> _glVersion{0, 0};
    bool _isRunning{false};

    std::unordered_map<std::string, std::shared_ptr<BaseObject>> _ghostObjects;

    // Gui exists in master scene whatever the configuration
    std::shared_ptr<Gui> _gui;
    bool _guiLinkedToWindow{false};

    // Default input objects
    std::shared_ptr<BaseObject> _keyboard{nullptr};
    std::shared_ptr<BaseObject> _mouse{nullptr};
    std::shared_ptr<BaseObject> _joystick{nullptr};
    std::shared_ptr<BaseObject> _dragndrop{nullptr};

// Objects in charge of calibration
#if HAVE_GPHOTO
    std::shared_ptr<ColorCalibrator> _colorCalibrator;
#endif

  private:
    static bool _isGlfwInitialized;

    std::shared_ptr<Scene> _self;
    bool _started{false};

    bool _isMaster{false}; //< Set to true if this is the master Scene of the current config
    bool _isInitialized{false};
    bool _status{false};  //< Set to true if an error occured during rendering
    int _swapInterval{1}; //< Global value for the swap interval, default for all windows

    // Texture upload context
    std::future<void> _textureUploadFuture;
    std::mutex _textureUploadMutex;
    std::condition_variable _textureUploadCondition;
    std::shared_ptr<GlWindow> _textureUploadWindow;
    std::atomic_bool _textureUploadDone{false};
    Spinlock _textureMutex; //!< Sync between texture and render loops
    GLsync _textureUploadFence, _cameraDrawnFence;

    // NV Swap group specific
    GLuint _maxSwapGroups{0};
    GLuint _maxSwapBarriers{0};

    unsigned long _nextId{0};

    //! Blender object
    std::shared_ptr<BaseObject> _blender{nullptr};

    /**
     * \brief Find which OpenGL version is available (from a predefined list)
     * \return Return MAJOR and MINOR
     */
    std::vector<int> findGLVersion();

    /**
     * \brief Set up the context and everything
     * \param name Scene name
     */
    void init(std::string name);

    /**
     * \brief Get the next available id
     * \return Returns a new id
     */
    unsigned long getId() { return ++_nextId; }

    /**
     * \brief Callback for GLFW errors
     * \param code Error code
     * \param msg Associated error message
     */
    static void glfwErrorCallback(int code, const char* msg);

/**
 * \brief Callback for GL errors and warnings
 */
#ifdef HAVE_OSX
    static void glMsgCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);
#else
    static void glMsgCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, void*);
#endif

    /**
     * \brief Texture update loop
     */
    void textureUploadRun();

    /**
     * \brief Register new attributes
     */
    void registerAttributes();

    /**
     * \brief Update the various inputs (mouse, keyboard...)
     */
    void updateInputs();
};

} // end of namespace

#endif // SPLASH_SCENE_H
