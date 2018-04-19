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

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/factory.h"
#if HAVE_PORTAUDIO
#include "./sound/ltcclock.h"
#endif
#include "./core/root_object.h"

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
     * \brief Constructor
     * \param argc Argument count
     * \param argv Arguments value
     */
    World(int argc, char** argv);

    /**
     * \brief Destructor
     */
    ~World() override;

    /**
     * \brief Get the status of the world after begin ran
     * \return Return true if all went well
     */
    bool getStatus() const { return !_status; }

    /**
     * \brief Run the world
     */
    void run();

  private:
    std::string _splashExecutable{"splash"};
    std::string _currentExePath{""};

#if HAVE_PORTAUDIO
    std::unique_ptr<LtcClock> _clock{nullptr}; //!< Master clock from a LTC signal
    std::string _clockDeviceName{""};          //!< Name of the input sound source for the master clock
#endif

    std::shared_ptr<Scene> _innerScene{}; //!< Inner Scene if the specified DISPLAY is available from this process
    std::thread _innerSceneThread;        //!< Inner Scene thread

    bool _status{true};        //!< Exit status
    bool _quit{false};         //!< True if the World should quit
    static World* _that;       //!< Pointer to the World
    struct sigaction _signals; //!< System signals
    std::string _executionPath{""};
    std::mutex _configurationMutex;
    bool _enforceCoreAffinity{false}; //!< If true, World and Scenes have their affinity fixed in specific, separate cores
    bool _enforceRealtime{false};     //!< If true, realtime scheduling is asked to the system, if possible

    // World parameters
    unsigned int _worldFramerate{60}; //!< World framerate, default 60, because synchronous tasks need the loop to run
    std::string _blendingMode{};      //!< Blending mode: can be none, once or continuous
    bool _runInBackground{false};     //!< If true, no window will be created

    bool _runAsChild{false}; //!< If true, runs as a child process
    std::string _childSceneName{"scene"};

    std::map<std::string, int> _scenes; //!< Map holding the PID of the Scene processes
    std::string _masterSceneName{""};   //!< Name of the master Scene
    std::string _displayServer{"0"};    //!< Display server.
    std::string _forcedDisplay{""};     //!< Set to force an output display
    bool _reloadingConfig{false};       // TODO: workaround to allow for correct reloading when an inner scene was used

    std::atomic_int _nextId{0};

    std::string _configFilename;  //!< Configuration file path
    std::string _projectFilename; //!< Project configuration file path
    Json::Value _config;          //!< Configuration as JSon

    bool _sceneLaunched{false};
    std::mutex _childProcessMutex;
    std::condition_variable _childProcessConditionVariable;

    // Synchronization testings
    int _swapSynchronizationTesting{0}; //!< If not 0, number of frames to keep the same color

    /**
     * \brief Add an object to the world (used for Images and Meshes currently)
     * \param type Object type
     * \param name Object name
     */
    void addToWorld(const std::string& type, const std::string& name);

    /**
     * \brief Apply the configuration
     */
    void applyConfig();

    /**
     * Spawn a scene given its parameters
     * \param name Scene name
     * \param display Display where to spawn the scene
     * \param address Address where to spawn the scene
     * \param spawn If true, the Scene is spawned, otherwise it is considered to be already running
     */
    bool addScene(const std::string& sceneName, const std::string& sceneDisplay, const std::string& sceneAddress, bool spawn = true);

    /**
     * \brief Copies the camera calibration from the given file to the current configuration
     * \param filename Source configuration file
     * \return Return true if everything went well
     */
    bool copyCameraParameters(const std::string& filename);

    /**
     * \brief Get a JSon string describing the attributes of all object types
     * \return Return a JSon string
     */
    std::string getObjectsAttributesDescriptions();

    /**
     * \brief Save the configuration
     */
    void saveConfig();

    /**
     * \brief Partially save the configuration
     * This saves only the modifications to images, textures and meshes
     * (in fact, all objects not related to projector calibration)
     */
    void saveProject();

    /**
     * \brief Get the next available id
     * \return Return an available id
     */
    int getId() { return _nextId.fetch_add(1); }

    /**
     * \brief Get the list of objects by their type
     * \param type Object type
     * \return Return a Values holding all the objects of the given type
     */
    Values getObjectsNameByType(const std::string& type);

    /**
     * \brief Redefinition of a method from RootObject. Send the input buffers back to all pairs
     * \param name Object name
     * \param obj Serialized object
     */
    void handleSerializedObject(const std::string& name, std::shared_ptr<SerializedObject> obj) override;

    /**
     * \brief Initializes the World
     */
    void init();

    /**
     * \brief Handle the exit signal messages
     */
    static void leave(int signal_value);

    /**
     * \brief Load the specified configuration file
     * \param filename Configuration file path
     * \param configuration JSon where the configuration will be stored
     * \return Return true if everything went well
     */
    bool loadConfig(const std::string& filename, Json::Value& configuration);

    /**
     * \brief Load a partial configuration file, updating existing configuration
     * \param filename Configuration file path
     * \return Return true if everything went well
     */
    bool loadProject(const std::string& filename);

    /**
     * \brief Parse the given arguments
     * \param argc Argument count
     * \param argv Argument values
     */
    void parseArguments(int argc, char** argv);

    /**
     * \brief Helper function to convert Json::Value to Splash::Values
     * \param values JSon to be processed
     * \return Return a Values converted from the JSon
     */
    Values jsonToValues(const Json::Value& values);

    /**
     * \brief Set a parameter for an object, given its name
     * \param name Object name
     * \param attrib Attribute name
     * \param args Value to set the attribute to
     */
    void setAttribute(const std::string& name, const std::string& attrib, const Values& args);
    using BaseObject::setAttribute;

    /**
     * \brief Callback for GLFW errors
     */
    static void glfwErrorCallback(int code, const char* msg);

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_WORLD_H
