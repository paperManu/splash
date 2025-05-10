/*
 * Copyright (C) 2013 Splash authors
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
#include <memory>
#include <vector>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/factory.h"
#include "./core/root_object.h"
#include "./core/spinlock.h"
#include "./graphics/object_library.h"
#include "./graphics/rendering_context.h"

namespace Splash
{

class ControllerObject;
class Gui;
class Scene;

/*************/
//! Scene class, which does the rendering on a given GPU
class Scene : public RootObject
{
    friend ControllerObject;
    friend UserInput;

  public:
    /**
     * Constructor
     * \param context Context for the creation of this Scene object
     */
    explicit Scene(Context context);

    /**
     *  Destructor
     */
    ~Scene() override;

    /**
     * Other constructors/operators
     */
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    /**
     *  Add an object of the given type, with the given name
     * \param type Object type
     * \param name Object name
     * \return Return a shared pointer to the created object
     */
    std::shared_ptr<GraphObject> addObject(const std::string& type, const std::string& name = "");

    /**
     *  Add an object ghosting one in another Scene. Used in master Scene for controlling purposes
     * \param type Object type
     * \param name Object name
     */
    void addGhost(const std::string& type, const std::string& name = "");

    /**
     * Get a reference to the object library
     * \return Return a reference to the object library
     */
    ObjectLibrary* getObjectLibrary() { return _objectLibrary.get(); }

    /**
     * Get a raw pointer to the Scene graphic renderer
     * \return Return a raw pointer to the renderer
     */
    gfx::Renderer* getRenderer() { return _renderer.get(); }

    /**
     *  Get the status of the scene
     * \return Return true if all is well
     */
    bool getStatus() const { return _status; }

    /**
     * Get the swap interval for this whole scene
     * \return Return the swap interval
     */
    int getSwapInterval() const { return _swapInterval; }

    /**
     *  Check whether it is initialized
     * \return Return true if the Scene is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     *  Ask whether the scene is Master or not
     * \return Return true if the Scene is master
     */
    bool isMaster() const { return _isMaster; }

    /**
     *  Check wether the scene is running
     * \return Return true if the scene runs
     */
    bool isRunning() const { return _isRunning; }

    /**
     *  Link an object to another, base on their types
     * \param first Child object
     * \param second Parent object
     * \return Return true if the linking succeeded
     */
    bool link(const std::string& first, const std::string& second);
    bool link(const std::shared_ptr<GraphObject>& first, const std::shared_ptr<GraphObject>& second);

    /**
     *  Unlink two objects. This always succeeds
     * \param first Child object
     * \param second Parent object
     */
    void unlink(const std::string& first, const std::string& second);
    void unlink(const std::shared_ptr<GraphObject>& first, const std::shared_ptr<GraphObject>& second);

    /**
     * Enable/disable user input from a joystick
     * \param enable Enable the joystick if true, disable otherwise
     */
    void setEnableJoystickInput(bool enable);

    /**
     * Get whether the joystick input is enabled or not
     * \return Return true if joystick input is enabled, false otherwise
     */
    bool getEnableJoystickInput() const;

    /**
     *  Remove an object
     * \param name Object name
     */
    void remove(const std::string& name);

    /**
     *  Render everything
     */
    void render();

    /**
     *  Main loop for the scene
     */
    void run();

    /**
     *  Set the Scene as the master one
     * \param configFilePath File path for the loaded configuration
     */
    void setAsMaster(const std::string& configFilePath = "");

    /**
     *  Set a message to be sent to the world
     * \param message Message type to send, which should correspond to a World attribute
     * \param value Message content
     */
    void sendMessageToWorld(const std::string& message, const Values& value = {});

    /**
     *  Set a message to be sent to the world, and wait for the World to send an answer
     * \param message Message type to send, which should correspond to a World attribute
     * \param value Message content
     * \param timeout Timeout in microseconds
     * \return Return the answer from the World
     */
    Values sendMessageToWorldWithAnswer(const std::string& message, const Values& value = {}, const unsigned long long timeout = 0);

  protected:
    std::atomic_bool _isRunning{false};

    // Gui exists in master scene whatever the configuration
    std::shared_ptr<Gui> _gui;

    // Default input objects
    std::shared_ptr<GraphObject> _keyboard{nullptr};
    std::shared_ptr<GraphObject> _mouse{nullptr};
    std::shared_ptr<GraphObject> _joystick{nullptr};
    std::shared_ptr<GraphObject> _dragndrop{nullptr};
    std::shared_ptr<GraphObject> _blender{nullptr};

// Objects in charge of calibration
#if HAVE_GPHOTO and HAVE_OPENCV
    std::shared_ptr<GraphObject> _colorCalibrator{nullptr};
#endif
#if HAVE_CALIMIRO
    std::shared_ptr<GraphObject> _geometricCalibrator{nullptr};
    std::shared_ptr<GraphObject> _texCoordGenerator{nullptr};
#endif

  private:
    std::unique_ptr<ObjectLibrary> _objectLibrary{nullptr};     //!< Library of 3D objects used by multiple GraphObjects
    static inline std::unique_ptr<gfx::Renderer> _renderer{nullptr}; // Must be static due to usage inside static functions in Scene.
    gfx::Renderer::RendererMsgCallbackData _rendererMsgCallbackData;

    bool _runInBackground{false}; //!< If true, no window will be created
    std::atomic_bool _started{false};

    bool _isMaster{false}; //!< Set to true if this is the master Scene of the current config
    bool _isInitialized{false};
    bool _status{false};                        //!< Set to true if an error occured during rendering
    int _swapInterval{1};                       //!< Global value for the swap interval, default for all windows
    unsigned long long _targetFrameDuration{0}; //!< Duration in microseconds of a frame at the refresh rate of the primary monitor
    std::atomic_bool _doUploadTextures{false};  //!< True if the render loop should upload the textures
    int64_t _lastSyncMessageDate{0};            //!< Time in µs a sync message was sent from World

    static std::vector<std::string> _ghostableTypes;

    /**
     *  Find which OpenGL version is available (from a predefined list)
     * \return Return MAJOR and MINOR
     */
    std::vector<int> findGLVersion();

    /**
     * Get a pointer to the rendering callback data, to be used by the rendering API error callback
     * \return Return a pointer to the rendering callback data
     */
    const gfx::Renderer::RendererMsgCallbackData* getRendererMsgCallbackDataPtr();

    /**
     *  Set up the context and everything
     * \param name Scene name
     */
    void init(const std::string& name);

    /**
     *  Computes and store the duration of a frame at the refresh rate of the primary monitor
     * \return The duration of a frame at the refresh rate of the primary monitor in microseconds
     */
    unsigned long long updateTargetFrameDuration();

    /**
     *  Register new attributes
     */
    void registerAttributes();

    /**
     * Initialize the tree
     */
    void initializeTree();

    /**
     *  Update the various inputs (mouse, keyboard...)
     */
    void updateInputs();
};

} // namespace Splash

#endif // SPLASH_SCENE_H
