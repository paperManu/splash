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
 * @world.h
 * The World class
 */

#ifndef SPLASH_WORLD_H
#define SPLASH_WORLD_H

#include <condition_variable>
#include <glm/glm.hpp>
#include <mutex>
#include <signal.h>
#include <string>
#include <thread>
#include <vector>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/factory.h"
#if HAVE_PORTAUDIO
#include "./sound/ltcclock.h"
#endif
#include "./core/root_object.h"
#include "./core/tree.h"

namespace Splash
{

class Scene;
class World;

/*************/
//! World class, responsible for setting and controlling the Scene, getting the frames (video or meshes) and dispatching them.
class World : public RootObject
{
  public:
    /**
     * Constructor
     * \param context Context for the creation of this World object
     */
    explicit World(Context context);

    /**
     * Destructor
     */
    ~World() override = default;

    /**
     * Other constructors/operators
     */
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    /**
     * Get the status of the world after begin ran
     * \return Return true if all went well
     */
    bool getStatus() const { return !_status; }

    /**
     * Run the world
     */
    void run();

  private:
#if HAVE_PORTAUDIO
    std::unique_ptr<LtcClock> _clock{nullptr}; //!< Master clock from a LTC signal
    std::string _clockDeviceName{""};          //!< Name of the input sound source for the master clock
#endif

    bool _status{true};        //!< Exit status
    bool _quit{false};         //!< True if the World should quit
    static World* _that;       //!< Pointer to the World
    struct sigaction _signals; //!< System signals
    std::mutex _configurationMutex;
    bool _enforceCoreAffinity{false}; //!< If true, World and Scenes have their affinity fixed in specific, separate cores
    bool _enforceRealtime{false};     //!< If true, realtime scheduling is asked to the system, if possible

    // World parameters
    unsigned int _worldFramerate{15}; //!< World framerate, default 60, because synchronous tasks need the loop to run
    std::string _blendingMode{};      //!< Blending mode: can be none, once or continuous
    bool _runInBackground{false};     //!< If true, no window will be created

    bool _runAsChild{false}; //!< If true, runs as a child process
    bool _spawnSubprocesses{true}; //!< If true, spawns subprocesses if needed
    std::string _childSceneName{"scene"};

    std::map<std::string, int> _scenes; //!< Map holding the PID of the Scene processes
    std::string _masterSceneName{""};   //!< Name of the master Scene

    std::string _configurationPath{""}; //!< Path to the configuration file
    std::string _mediaPath{""};         //!< Default path to the medias
    std::string _configFilename;        //!< Configuration file path
    std::string _projectFilename;       //!< Project configuration file path
    Json::Value _config;                //!< Configuration as JSon

    NameRegistry _nameRegistry{}; //!< Object name registry
    bool _sceneLaunched{false};
    std::mutex _childProcessMutex;
    std::condition_variable _childProcessConditionVariable;

    // Synchronization testings
    int _swapSynchronizationTesting{0}; //!< If not 0, number of frames to keep the same color

    /**
     * Add an object to the world (used for Images and Meshes currently)
     * \param type Object type
     * \param name Object name
     */
    void addToWorld(const std::string& type, const std::string& name);

    /**
     * Match the current context
     * \return Return true if the context was applied successfully
     */
    bool applyContext();

    /**
     * Apply the configuration
     * \return Return true if the configuration was applied successfully
     */
    bool applyConfig();

    /**
     * Spawn a scene given its parameters
     * \param name Scene name
     * \param display Display where to spawn the scene
     * \param address Address where to spawn the scene
     * \param spawn If true, the Scene is spawned, otherwise it is considered to be already running
     */
    bool addScene(const std::string& sceneName, const std::string& sceneDisplay, const std::string& sceneAddress, bool spawn = true);

    /**
     * Copies the camera calibration from the given file to the current configuration
     * \param filename Source configuration file
     * \return Return true if everything went well
     */
    bool copyCameraParameters(const std::string& filename);

    /**
     * Get a JSon string describing the attributes of all object types
     * \return Return a JSon string
     */
    std::string getObjectsAttributesDescriptions();

    /**
     * Save the configuration
     */
    void saveConfig();

    /**
     * Partially save the configuration
     * This saves only the modifications to images, textures and meshes
     * (in fact, all objects not related to projector calibration)
     * \param filename Path to the project file
     */
    void saveProject(const std::string& filename);

    /**
     * Get all object of given type.
     * \param type Type to look for. If empty, get all objects.
     * \return Return a list of all objects of the given type
     */
    std::vector<std::string> getObjectsOfType(const std::string& type) const;

    /**
     * Redefinition of a method from RootObject. Send the input buffers back to all pairs
     *
     * Note that depending on whether the object can be handled or not, it will be
     * made invalid after calling this method. Check the return value to get whether
     * the object is still valid.
     *
     * \param name Object name
     * \param obj Serialized object
     * \return Return true if the object has been handled
     */
    bool handleSerializedObject(const std::string& name, SerializedObject& obj) override;

    /**
     * Handle the exit signal messages
     */
    static void leave(int signal_value);

    /**
     * Load the specified configuration file
     * \param filename Configuration file path
     * \param configuration JSon where the configuration will be stored
     * \return Return true if everything went well
     */
    bool loadConfig(const std::string& filename, Json::Value& configuration);

    /**
     * Load a partial configuration file, updating existing configuration
     * \param filename Configuration file path
     * \return Return true if everything went well
     */
    bool loadProject(const std::string& filename);

    /**
     * Callback for GLFW errors
     */
    static void glfwErrorCallback(int code, const char* msg);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

    /**
     * Initialize the tree
     */
    void initializeTree();
};

} // namespace Splash

#endif // SPLASH_WORLD_H
