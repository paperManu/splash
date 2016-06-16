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
#include <mutex>
#include <signal.h>
#include <string>
#include <thread>
#include <vector>
#include <glm/glm.hpp>

#include "./config.h"

#include "./coretypes.h"
#include "./basetypes.h"
#include "./factory.h"
#if HAVE_PORTAUDIO
    #include "./ltcclock.h"
#endif
#include "./queue.h"

namespace Splash {

class Scene;
class World;
typedef std::shared_ptr<World> WorldPtr;

/*************/
class World : public RootObject
{
    friend Queue;

    public:
        /**
         * Constructor
         */
        World(int argc, char** argv);

        /**
         * Destructor
         */
        ~World();

        /**
         * Get a JSon string describing the attributes of all object types
         */
        std::string getObjectsAttributesDescriptions();

        /**
         * Get the status of the world
         */
        bool getStatus() const {return !_status;}

        /**
         * Run the world
         */
        void run();

    private:
        WorldPtr _self;
#if HAVE_PORTAUDIO
        std::unique_ptr<LtcClock> _clock {nullptr};
        std::string _clockDeviceName {""};
#endif

        std::unique_ptr<Factory> _factory {nullptr};

        std::shared_ptr<Scene> _innerScene {};
        std::thread _innerSceneThread;

        bool _status {true};
        bool _quit {false};
        static World* _that;
        struct sigaction _signals;
        std::string _executionPath {""};
        std::mutex _configurationMutex;

        // World parameters
        unsigned int _worldFramerate {60};
        std::string _blendingMode {};

        std::map<std::string, int> _scenes;
        std::string _masterSceneName {""};
        bool _reloadingConfig {false}; // TODO: workaround to allow for correct reloading when an inner scene was used

        std::atomic_int _nextId {0};
        std::map<std::string, std::vector<std::string>> _objectDest;

        std::string _configFilename;
        Json::Value _config;

        bool _sceneLaunched {false};
        std::mutex _childProcessMutex;
        std::condition_variable _childProcessConditionVariable;
        
        // Synchronization testings
        int _swapSynchronizationTesting {0}; // If not 0, number of frames to keep the same color

        /**
         * Add an object to the world (used for Images and Meshes currently)
         */
        void addLocally(std::string type, std::string name, std::string destination);

        /**
         * Apply the configuration
         */
        void applyConfig();

        /**
         * Copies the camera calibration from the given file to the current configuration
         */
        bool copyCameraParameters(std::string filename);

        /**
         * Save the configuration
         */
        void saveConfig();

        /**
         * Get the next available id
         */
        int getId() {return _nextId.fetch_add(1);}

        /**
         * Get the list of objects by their type
         */
        Values getObjectsNameByType(std::string type);

        /**
         * Redefinition of a method from RootObject
         * Send the input buffers back to all pairs
         */
        void handleSerializedObject(const std::string name, std::shared_ptr<SerializedObject> obj);

        /**
         * Initialize the GLFW window
         */
        void init();

        /**
         * Handle the exit signal messages
         */
        static void leave(int signal_value);

        /**
         * Load the specified configuration file
         */
        bool loadConfig(std::string filename, Json::Value& configuration);

        /**
         * Parse the given arguments
         */
        void parseArguments(int argc, char** argv);

        /**
         * Helper function to read Json arrays
         */
        Values processArray(Json::Value values);

        /**
         * Set a parameter for an object, given its id
         */
        void setAttribute(std::string name, std::string attrib, const Values& args);
        using BaseObject::setAttribute;

        /**
         * Callback for GLFW errors
         */
        static void glfwErrorCallback(int code, const char* msg);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

 #endif // SPLASH_WORLD_H
