#include "world.h"

#include <chrono>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <regex>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "./image.h"
#include "./link.h"
#include "./log.h"
#include "./mesh.h"
#include "./osUtils.h"
#include "./queue.h"
#include "./scene.h"
#include "./threadpool.h"
#include "./timer.h"

// Included only for creating the documentation through the --info flag
#include "./geometry.h"
#include "./window.h"

using namespace glm;
using namespace std;

namespace Splash
{
/*************/
World* World::_that;

/*************/
World::World(int argc, char** argv)
{
    init();

    parseArguments(argc, argv);
}

/*************/
World::~World()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "World::~World - Destructor" << Log::endl;
#endif
    if (_innerSceneThread.joinable())
        _innerSceneThread.join();

    // This has to be done last. Any object using the threadpool should be destroyed before
    SThread::pool.waitAllThreads();
}

/*************/
void World::run()
{
    applyConfig();

    while (true)
    {
        Timer::get() << "worldLoop";
        Timer::get() << "innerWorldLoop";
        lock_guard<mutex> lockConfiguration(_configurationMutex);

        // Execute waiting tasks
        runTasks();

        {
            lock_guard<recursive_mutex> lockObjects(_objectsMutex);

            // Read and serialize new buffers
            Timer::get() << "serialize";
            vector<unsigned int> threadIds;
            unordered_map<string, shared_ptr<SerializedObject>> serializedObjects;
            for (auto& o : _objects)
            {
                auto bufferObj = dynamic_pointer_cast<BufferObject>(o.second);
                // This prevents the map structure to be modified in the threads
                auto serializedObjectIt = serializedObjects.emplace(std::make_pair(bufferObj->getDistantName(), shared_ptr<SerializedObject>(nullptr)));
                if (!serializedObjectIt.second)
                    continue; // Error while inserting the object in the map

                threadIds.push_back(SThread::pool.enqueue([=, &o]() {
                    // Update the local objects
                    o.second->update();

                    // Send them the their destinations
                    if (bufferObj.get() != nullptr)
                    {
                        if (bufferObj->wasUpdated()) // if the buffer has been updated
                        {
                            auto obj = bufferObj->serialize();
                            bufferObj->setNotUpdated();
                            if (obj)
                                serializedObjectIt.first->second = obj;
                        }
                    }
                }));
            }
            SThread::pool.waitThreads(threadIds);
            Timer::get() >> "serialize";

            // Wait for previous buffers to be uploaded
            _link->waitForBufferSending(chrono::milliseconds((unsigned long long)(1e3 / 30))); // Maximum time to wait for frames to arrive
            sendMessage(SPLASH_ALL_PEERS, "bufferUploaded", {});
            Timer::get() >> "upload";

            // Ask for the upload of the new buffers, during the next world loop
            Timer::get() << "upload";
            for (auto& o : serializedObjects)
                if (o.second)
                    _link->sendBuffer(o.first, std::move(o.second));
        }

        // Update the distant attributes
        for (auto& o : _objects)
        {
            auto attribs = o.second->getDistantAttributes();
            for (auto& attrib : attribs)
            {
                sendMessage(o.second->getName(), attrib.first, attrib.second);
            }
        }

        // If the master scene is not an inner scene, we have to send it some information
        if (_scenes[_masterSceneName] != -1)
        {
            // Send current timings to all Scenes, for display purpose
            auto& durationMap = Timer::get().getDurationMap();
            for (auto& d : durationMap)
                sendMessage(_masterSceneName, "duration", {d.first, (int)d.second});
            // Also send the master clock if needed
            Values clock;
            if (Timer::get().getMasterClock(clock))
                sendMessage(_masterSceneName, "masterClock", clock);

            // Send newer logs to all master Scene
            auto logs = Log::get().getNewLogs();
            for (auto& log : logs)
                sendMessage(_masterSceneName, "log", {log.first, (int)log.second});
        }

        if (_quit)
        {
            for (auto& s : _scenes)
                sendMessage(s.first, "quit", {});
            break;
        }

        // Sync with buffer object update
        Timer::get() >> "innerWorldLoop";
        auto elapsed = Timer::get().getDuration("innerWorldLoop");
        waitSignalBufferObjectUpdated(1e6 / (float)_worldFramerate - elapsed);

        // Sync to world framerate
        Timer::get() >> "worldLoop";
    }
}

/*************/
void World::addLocally(const string& type, const string& name, const string& destination)
{
    // Images and Meshes have a counterpart on this side
    if (type.find("image") == string::npos && type.find("mesh") == string::npos && type.find("queue") == string::npos)
        return;

    auto object = _factory->create(type);
    auto realName = name;
    if (object.get() != nullptr)
    {
        object->setId(getId());
        realName = object->setName(name); // The real name is not necessarily the one we set (see Queues)
        _objects[realName] = object;
    }

    // If the object is not registered yet, we add it with the specified destination
    // as well as the WORLD_SCENE destination
    if (_objectDest.find(realName) == _objectDest.end())
    {
        _objectDest[realName] = vector<string>();
        _objectDest[realName].emplace_back(destination);
    }
    // If it is, we only add the new destination
    else
    {
        bool isPresent = false;
        for (auto d : _objectDest[realName])
            if (d == destination)
                isPresent = true;
        if (!isPresent)
        {
            _objectDest[realName].emplace_back(destination);
        }
    }
}

/*************/
void World::applyConfig()
{
    lock_guard<mutex> lockConfiguration(_configurationMutex);

    // We first destroy all scene and objects
    _scenes.clear();
    _objects.clear();
    _objectDest.clear();
    _masterSceneName = "";

    try
    {
        // Get the list of all scenes, and create them
        if (!_config.isMember("scenes"))
        {
            Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Error while getting scenes configuration" << Log::endl;
            return;
        }

        const Json::Value jsScenes = _config["scenes"];

        for (int i = 0; i < jsScenes.size(); ++i)
        {
            // If no address has been specified, we consider it is localhost
            if (!jsScenes[i].isMember("address") || jsScenes[i]["address"].asString() == string("localhost"))
            {
                if (!jsScenes[i].isMember("name"))
                {
                    Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Scenes need a name" << Log::endl;
                    return;
                }
                int spawn = 1;
                if (jsScenes[i].isMember("spawn"))
                    spawn = jsScenes[i]["spawn"].asInt();

                string display{""};
#if HAVE_LINUX
                auto regDisplayFull = regex("(:[0-9]\\.[0-9])", regex_constants::extended);
                auto regDisplayInt = regex("[0-9]", regex_constants::extended);
                smatch match;

                display = "DISPLAY=:0.0";
                if (jsScenes[i].isMember("display"))
                {
                    auto displayParameter = jsScenes[i]["display"].asString();
                    if (regex_match(displayParameter, match, regDisplayFull))
                        display = "DISPLAY=" + displayParameter;
                    else if (regex_match(displayParameter, match, regDisplayInt))
                        display = "DISPLAY=:0." + displayParameter;
                }

                if (!_forcedDisplay.empty())
                {
                    if (regex_match(_forcedDisplay, match, regDisplayFull))
                        display = "DISPLAY=" + _forcedDisplay;
                    else if (regex_match(_forcedDisplay, match, regDisplayInt))
                        display = "DISPLAY=:0." + _forcedDisplay;
                }
#endif

                string name = jsScenes[i]["name"].asString();
                int pid = -1;
                if (spawn > 0)
                {
                    _sceneLaunched = false;
                    string worldDisplay = "none";
#if HAVE_LINUX
                    if (getenv("DISPLAY"))
                    {
                        worldDisplay = getenv("DISPLAY");
                        if (worldDisplay.size() == 2)
                            worldDisplay += ".0";
                        if (_reloadingConfig)
                            worldDisplay = "none";
                    }
#endif

                    // If the current process is on the correct display, we use an inner Scene
                    if (worldDisplay.size() > 0 && display.find(worldDisplay) == display.size() - worldDisplay.size() && !_innerScene)
                    {
                        Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Starting an inner Scene" << Log::endl;
                        _innerScene = make_shared<Scene>(name);
                        _innerSceneThread = thread([&]() { _innerScene->run(); });
                    }
                    else
                    {
                        // Spawn a new process containing this Scene
                        Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Starting a Scene in another process" << Log::endl;

                        string cmd;
                        if (_executionPath == "")
                            cmd = string(SPLASHPREFIX) + "/bin/splash-scene";
                        else
                            cmd = _executionPath + "splash-scene";
                        string debug = (Log::get().getVerbosity() == Log::DEBUGGING) ? "-d" : "";
                        string timer = Timer::get().isDebug() ? "-t" : "";
                        string xauth = "XAUTHORITY=" + Utils::getHomePath() + "/.Xauthority";

                        char* argv[] = {(char*)cmd.c_str(), (char*)debug.c_str(), (char*)timer.c_str(), (char*)name.c_str(), NULL};
                        char* env[] = {(char*)display.c_str(), (char*)xauth.c_str(), NULL};
                        int status = posix_spawn(&pid, cmd.c_str(), NULL, NULL, argv, env);
                        if (status != 0)
                            Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Error while spawning process for scene " << name << Log::endl;
                    }

                    // We wait for the child process to be launched
                    unique_lock<mutex> lockChildProcess(_childProcessMutex);
                    while (!_sceneLaunched)
                    {
                        if (cv_status::timeout == _childProcessConditionVariable.wait_for(lockChildProcess, chrono::seconds(5)))
                        {
                            Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Timeout when trying to connect to newly spawned scene \"" << name << "\". Exiting."
                                       << Log::endl;
                            _quit = true;
                            return;
                        }
                    }
                }

                _scenes[name] = pid;
                if (_masterSceneName == "")
                    _masterSceneName = name;

                // Initialize the communication
                if (pid == -1 && spawn)
                    _link->connectTo(name, _innerScene.get());
                else
                    _link->connectTo(name);

                // Set the remaining parameters
                auto sceneMembers = jsScenes[i].getMemberNames();
                int idx{0};
                for (const auto& param : jsScenes[i])
                {
                    string paramName = sceneMembers[idx];

                    auto values = jsonToValues(param);
                    sendMessage(name, paramName, values);
                    idx++;
                }
            }
            else
            {
                Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Non-local scenes are not implemented yet" << Log::endl;
            }
        }

        // Configure each scenes
        // The first scene is the master one, and also receives some ghost objects
        // First, we create the objects
        for (auto& s : _scenes)
        {
            if (!_config.isMember(s.first))
                continue;

            const Json::Value jsScene = _config[s.first];

            // Set if master
            if (s.first == _masterSceneName)
                sendMessage(_masterSceneName, "setMaster", {_configFilename});

            // Create the objects
            auto sceneMembers = jsScene.getMemberNames();
            int idx = 0;
            for (const auto& obj : jsScene)
            {
                string name = sceneMembers[idx];
                if (name == "links" || !obj.isMember("type"))
                {
                    idx++;
                    continue;
                }

                string type = obj["type"].asString();
                if (type != "scene")
                {
                    sendMessage(s.first, "add", {type, name});
                    if (s.first != _masterSceneName)
                        sendMessage(_masterSceneName, "addGhost", {type, name});

                    // Some objects are also created on this side, and linked with the distant one
                    addLocally(type, name, s.first);
                }

                idx++;
            }

            // Set some default directories
            sendMessage(SPLASH_ALL_PEERS, "configurationPath", {_configurationPath});
            sendMessage(SPLASH_ALL_PEERS, "mediaPath", {_configurationPath});
            sendMessage(SPLASH_ALL_PEERS, "runInBackground", {_runInBackground});
        }

        // Then we link the objects together
        for (auto& s : _scenes)
        {
            if (!_config.isMember(s.first))
                continue;

            const Json::Value jsScene = _config[s.first];
            auto sceneMembers = jsScene.getMemberNames();

            int idx = 0;
            for (const auto& obj : jsScene)
            {
                if (sceneMembers[idx] != "links")
                {
                    idx++;
                    continue;
                }

                for (auto& link : obj)
                {
                    if (link.size() < 2)
                        continue;
                    sendMessage(s.first, "link", {link[0].asString(), link[1].asString()});
                    if (s.first != _masterSceneName)
                        sendMessage(_masterSceneName, "linkGhost", {link[0].asString(), link[1].asString()});
                }
                idx++;
            }
        }

        // Configure the objects
        for (auto& s : _scenes)
        {
            if (!_config.isMember(s.first))
                continue;

            const Json::Value jsScene = _config[s.first];

            // Create the objects
            auto sceneMembers = jsScene.getMemberNames();
            int idx{0};
            for (const auto& obj : jsScene)
            {
                string name = sceneMembers[idx];
                if (name == "links" || !obj.isMember("type"))
                {
                    idx++;
                    continue;
                }

                string type = obj["type"].asString();

                // Set their attributes
                auto objMembers = obj.getMemberNames();
                int idxAttr = 0;
                for (const auto& attr : obj)
                {
                    if (objMembers[idxAttr] == "type")
                    {
                        idxAttr++;
                        continue;
                    }

                    auto values = jsonToValues(attr);

                    // Send the attribute
                    sendMessage(name, objMembers[idxAttr], values);
                    if (type != "scene")
                    {
                        // We also the attribute locally, if the object exists
                        set(name, objMembers[idxAttr], values, false);

                        // The attribute is also sent to the master scene
                        if (s.first != _masterSceneName)
                        {
                            values.push_front(objMembers[idxAttr]);
                            values.push_front(name);
                            sendMessage(_masterSceneName, "setGhost", values);
                        }
                    }

                    idxAttr++;
                }

                idx++;
            }
        }

        // Lastly, configure this very World
        // This happens last as some parameters are sent to Scenes (like blending computation)
        if (_config.isMember("world"))
        {
            const Json::Value jsWorld = _config["world"];
            auto worldMember = jsWorld.getMemberNames();
            int idx{0};
            for (const auto& attr : jsWorld)
            {
                auto values = jsonToValues(attr);
                string paramName = worldMember[idx];
                setAttribute(paramName, values);
                idx++;
            }
        }
    }
    catch (...)
    {
        Log::get() << Log::ERROR << "Exception caught while applying configuration from file " << _configFilename << Log::endl;
        return;
    }

// Also, enable the master clock if it was not enabled
#if HAVE_PORTAUDIO
    addTask([&]() {
        if (!_clock)
            _clock = unique_ptr<LtcClock>(new LtcClock(true));
    });
#endif

    // Send the start message for all scenes
    for (auto& s : _scenes)
    {
        auto answer = sendMessageWithAnswer(s.first, "start", {}, 2e6);
        if (0 == answer.size())
        {
            Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Timeout when trying to connect to scene \"" << s.first << "\". Exiting." << Log::endl;
            _quit = true;
            break;
        }
    }
}

/*************/
string World::getObjectsAttributesDescriptions()
{
    Json::Value root;

    auto formatDescription = [](const string desc, const Values& argTypes) -> string {
        string descriptionStr = "[";
        for (int i = 0; i < argTypes.size(); ++i)
        {
            descriptionStr += argTypes[i].as<string>();
            if (i < argTypes.size() - 1)
                descriptionStr += ", ";
        }
        descriptionStr += "] " + desc;

        return descriptionStr;
    };

    // We create "fake" objects and ask then for their attributes
    auto localFactory = Factory();
    auto types = localFactory.getObjectTypes();
    for (auto& type : types)
    {
        auto obj = localFactory.create(type);
        if (!obj)
            continue;

        auto objectDescription = localFactory.getDescription(type);
        root[obj->getType() + "_description"] = objectDescription;

        auto attributesDescriptions = obj->getAttributesDescriptions();
        int addedAttribute = 0;
        root[obj->getType()] = Json::Value();
        for (auto& d : attributesDescriptions)
        {
            // We only keep attributes with a valid documentation
            // The other ones are inner attributes
            if (d[1].as<string>().size() == 0)
                continue;

            // We also don't keep attributes with no argument types
            if (d[2].as<Values>().size() == 0)
                continue;

            root[obj->getType()][d[0].as<string>()] = formatDescription(d[1].as<string>(), d[2].as<Values>());

            addedAttribute++;
        }

        // If the object has no documented attribute
        if (addedAttribute == 0)
            root.removeMember(obj->getType());
    }

    // Also, add documentation for the World and Scene types
    auto worldDescription = getAttributesDescriptions();
    for (auto& d : worldDescription)
    {
        if (d[1].size() == 0)
            continue;

        root["world"][d[0].as<string>()] = formatDescription(d[1].as<string>(), d[2].as<Values>());
    }

    setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas
    string jsonString;
    jsonString = root.toStyledString();

    return jsonString;
}

/*************/
void World::saveConfig()
{
    setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas

    Json::Value root;
    root["scenes"] = Json::Value();

    // Get the configuration from the different scenes
    for (auto& s : _scenes)
    {
        Json::Value scene;
        scene["name"] = s.first;
        scene["address"] = "localhost"; // Distant scenes are not yet supported
        root["scenes"].append(scene);

        // Get this scene's configuration
        Values answer = sendMessageWithAnswer(s.first, "config");

        // Parse the string to get a json
        Json::Value config;
        Json::Reader reader;
        reader.parse(answer[2].as<string>(), config);
        root[s.first] = config;
    }

    // Local objects configuration can differ from the scenes objects,
    // as their type is not necessarily identical
    Json::Value& jsScenes = _config["scenes"];
    for (int i = 0; i < jsScenes.size(); ++i)
    {
        string sceneName = jsScenes[i]["name"].asString();
        _config[sceneName] = Json::Value();

        // Set the scene configuration from what was received in the previous loop
        Json::Value::Members attributes = root[sceneName][sceneName].getMemberNames();
        for (const auto& attr : attributes)
            jsScenes[i][attr] = root[sceneName][sceneName][attr];

        if (root.isMember(sceneName))
        {
            Json::Value& scene = root[sceneName];
            Json::Value::Members members = scene.getMemberNames();

            for (auto& m : members)
            {
                // The root objects contains configuration for the scenes as if they
                // were Objects themselves, although they are RootObjects. We should not
                // include them here
                if (m == sceneName)
                    continue;

                if (!_config[sceneName].isMember(m))
                    _config[sceneName][m] = Json::Value();

                if (m != "links")
                {
                    attributes = scene[m].getMemberNames();
                    for (const auto& a : attributes)
                        _config[sceneName][m][a] = scene[m][a];

                    const auto& obj = _objects.find(m);
                    if (obj != _objects.end())
                    {
                        Json::Value worldObjValue = obj->second->getConfigurationAsJson();
                        attributes = worldObjValue.getMemberNames();
                        for (const auto& a : attributes)
                            _config[sceneName][m][a] = worldObjValue[a];
                    }
                }
                else
                {
                    _config[sceneName][m] = scene[m];
                }
            }
        }
    }

    // Configuration from the world
    _config["description"] = SPLASH_FILE_CONFIGURATION;
    auto worldConfiguration = BaseObject::getConfigurationAsJson();
    auto attributes = worldConfiguration.getMemberNames();
    for (const auto& attr : attributes)
    {
        _config["world"][attr] = worldConfiguration[attr];
    }

    setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas
    ofstream out(_configFilename, ios::binary);
    out << _config.toStyledString();
    out.close();
}

/*************/
void World::saveProject()
{
    try
    {
        setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas

        auto root = Json::Value(); // Haha, auto root...
        root["description"] = SPLASH_FILE_PROJECT;
        root["links"] = Json::Value();

        // Here, we don't care about which Scene holds which object, as objects with the
        // same name in different Scenes are necessarily clones
        for (auto& s : _scenes)
        {
            // Get this scene's configuration
            Values answer = sendMessageWithAnswer(s.first, "config");

            // Parse the string to get a json
            Json::Value config;
            Json::Reader reader;
            reader.parse(answer[2].as<string>(), config);

            auto members = config.getMemberNames();
            for (const auto& m : members)
            {
                if (m == "links")
                {
                    for (auto& v : config[m])
                        root[m].append(v);
                    continue;
                }

                if (!config[m].isMember("type"))
                    continue;

                // We only save configuration for non Scene-specific objects, which are one of the following:
                bool isSavableType = false;
                for (const auto& type : _partiallySavableTypes)
                {
                    if (config[m]["type"].asString().find(type) != string::npos)
                        isSavableType = true;
                }

                if (!isSavableType)
                    continue;

                root[m] = Json::Value();
                auto attributes = config[m].getMemberNames();
                for (const auto& a : attributes)
                    root[m][a] = config[m][a];

                // Check for configuration of this object held in the World context
                const auto& obj = _objects.find(m);
                if (obj != _objects.end())
                {
                    Json::Value worldObjValue = obj->second->getConfigurationAsJson();
                    attributes = worldObjValue.getMemberNames();
                    for (const auto& a : attributes)
                        root[m][a] = worldObjValue[a];
                }
            }
        }

        ofstream out(_projectFilename, ios::binary);
        out << root.toStyledString();
        out.close();
    }
    catch (...)
    {
        Log::get() << Log::ERROR << "Exception caught while saving file " << _projectFilename << Log::endl;
    }
}

/*************/
Values World::getObjectsNameByType(const string& type)
{
    Values answer = sendMessageWithAnswer(_masterSceneName, "getObjectsNameByType", {type});
    return answer[2].as<Values>();
}

/*************/
void World::handleSerializedObject(const string& name, shared_ptr<SerializedObject> obj)
{
    _link->sendBuffer(name, std::move(obj));
}

/*************/
void World::init()
{
    _type = "world";
    _name = "world";

    _that = this;
    _signals.sa_handler = leave;
    _signals.sa_flags = 0;
    sigaction(SIGINT, &_signals, NULL);
    sigaction(SIGTERM, &_signals, NULL);

    _link = make_shared<Link>(this, _name);

    registerAttributes();
}

/*************/
void World::leave(int signal_value)
{
    Log::get() << "World::" << __FUNCTION__ << " - Received a SIG event. Quitting." << Log::endl;
    _that->_quit = true;
}

/*************/
bool World::copyCameraParameters(const std::string& filename)
{
    ifstream in(filename, ios::in | ios::binary);
    string contents;
    if (in)
    {
        in.seekg(0, ios::end);
        contents.resize(in.tellg());
        in.seekg(0, ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
    else
    {
        Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to open file " << filename << Log::endl;
        return false;
    }

    Json::Value config;
    Json::Reader reader;

    bool success = reader.parse(contents, config);
    if (!success)
    {
        Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to parse file " << filename << Log::endl;
        Log::get() << Log::WARNING << reader.getFormattedErrorMessages() << Log::endl;
        return false;
    }

    // Get the scene names from this other configuration file
    const Json::Value jsScenes = config["scenes"];
    vector<string> sceneNames;
    for (int i = 0; i < jsScenes.size(); ++i)
        if (jsScenes[i].isMember("name"))
            sceneNames.push_back(jsScenes[i]["name"].asString());

    for (const auto& s : sceneNames)
    {
        if (!config.isMember(s))
            continue;

        const Json::Value jsScene = config[s];
        auto sceneMembers = jsScene.getMemberNames();
        int idx = 0;
        // Look for the cameras in the configuration file
        for (const auto& obj : jsScene)
        {
            string name = sceneMembers[idx];
            if (name == "links" || !obj.isMember("type"))
            {
                idx++;
                continue;
            }

            if (obj["type"].asString() != "camera")
            {
                idx++;
                continue;
            }

            // Go through the camera attributes
            auto objMembers = obj.getMemberNames();
            int idxAttr = 0;
            for (const auto& attr : obj)
            {
                if (objMembers[idxAttr] == "type")
                {
                    idxAttr++;
                    continue;
                }

                auto values = jsonToValues(attr);

                // Send the new values for this attribute
                sendMessage(name, objMembers[idxAttr], values);

                // Also send it to a ghost if it exists
                values.push_front(objMembers[idxAttr]);
                values.push_front(name);
                sendMessage(_masterSceneName, "setGhost", values);

                idxAttr++;
            }

            idx++;
        }
    }

    return true;
}

/*************/
Values World::jsonToValues(const Json::Value& values)
{
    Values outValues;

    if (values.isInt())
        outValues.emplace_back(values.asInt());
    else if (values.isDouble())
        outValues.emplace_back(values.asFloat());
    else if (values.isArray())
        for (const auto& v : values)
        {
            if (v.isInt())
                outValues.emplace_back(v.asInt());
            else if (v.isDouble())
                outValues.emplace_back(v.asFloat());
            else if (v.isArray())
                outValues.emplace_back(jsonToValues(v));
            else
                outValues.emplace_back(v.asString());
        }
    else
        outValues.emplace_back(values.asString());

    return outValues;
}

/*************/
bool World::loadJsonFile(const string& filename, Json::Value& configuration)
{
    ifstream in(filename, ios::in | ios::binary);
    string contents;
    if (in)
    {
        in.seekg(0, ios::end);
        contents.resize(in.tellg());
        in.seekg(0, ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
    else
    {
        Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to open file " << filename << Log::endl;
        return false;
    }

    Json::Value config;
    Json::Reader reader;

    bool success = reader.parse(contents, config);
    if (!success)
    {
        Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to parse file " << filename << Log::endl;
        Log::get() << Log::WARNING << reader.getFormattedErrorMessages() << Log::endl;
        return false;
    }

    configuration = config;
    return true;
}

/*************/
bool World::loadConfig(const string& filename, Json::Value& configuration)
{
    if (!loadJsonFile(filename, configuration))
        return false;

    if (!configuration.isMember("description") || configuration["description"].asString() != SPLASH_FILE_CONFIGURATION)
        return false;

    _configFilename = filename;
    _configurationPath = Utils::getPathFromFilePath(_configFilename);
    _mediaPath = _configurationPath; // By default, same directory
    return true;
}

/*************/
bool World::loadProject(const string& filename)
{
    try
    {
        Json::Value partialConfig;
        if (!loadJsonFile(filename, partialConfig))
            return false;

        if (!partialConfig.isMember("description") || partialConfig["description"].asString() != SPLASH_FILE_PROJECT)
            return false;

        _projectFilename = filename;

        // Now, we apply the configuration depending on the current state
        // Meaning, we replace objects with the same name, create objects with non-existing name,
        // and delete objects which are not in the partial config
        auto& scenes = _config["scenes"];

        // Delete existing objects
        for (const auto& s : _scenes)
        {
            auto members = _config[s.first].getMemberNames();
            for (const auto& m : members)
            {
                if (m == "links")
                    continue;
                if (!_config[s.first][m].isMember("type"))
                    continue;

                bool isSavableType = false;
                for (const auto& type : _partiallySavableTypes)
                    if (_config[s.first][m]["type"].asString().find(type) != string::npos)
                        isSavableType = true;

                if (!isSavableType)
                    continue;

                setAttribute("deleteObject", {m});
            }
        }

        // Create new objects
        auto members = partialConfig.getMemberNames();
        for (const auto& m : members)
        {
            if (m == "links" || m == "description")
                continue;
            if (!partialConfig[m].isMember("type"))
                continue;
            setAttribute("addObject", {partialConfig[m]["type"].asString(), m});
        }

        // Handle the links
        if (partialConfig.isMember("links"))
        {
            for (const auto& link : partialConfig["links"])
            {
                if (link.size() != 2)
                    continue;
                auto source = link[0].asString();
                auto sink = link[1].asString();

                addTask([=]() {
                    sendMessage(SPLASH_ALL_PEERS, "link", {link[0].asString(), link[1].asString()});
                    sendMessage(SPLASH_ALL_PEERS, "linkGhost", {link[0].asString(), link[1].asString()});
                });
            }
        }

        // Configure the objects
        for (const auto& name : members)
        {
            if (name == "links" || name == "description")
                continue;

            auto& obj = partialConfig[name];
            string type = obj["type"].asString();

            if (type == "scene")
                continue;

            // Before anything, all objects have the right to know what the current path is
            auto path = Utils::getPathFromFilePath(_configFilename);
            sendMessage(name, "configFilePath", {path});
            sendMessage(_masterSceneName, "setGhost", {name, "configFilePath", path});
            set(name, "configFilePath", {path}, false);

            // Set their attributes
            auto objMembers = obj.getMemberNames();
            int idxAttr = 0;
            for (const auto& attr : obj)
            {
                if (objMembers[idxAttr] == "type")
                {
                    idxAttr++;
                    continue;
                }

                auto values = jsonToValues(attr);
                values.push_front(objMembers[idxAttr]);
                values.push_front(name);
                setAttribute("sendAll", values);

                idxAttr++;
            }
        }

        return true;
    }
    catch (...)
    {
        Log::get() << Log::ERROR << "Exception caught while loading file " << filename << Log::endl;
        return false;
    }
}

/*************/
void World::parseArguments(int argc, char** argv)
{
    cout << endl;
    cout << "\t             \033[33;1m- Splash -\033[0m" << endl;
    cout << "\t\033[1m- Modular multi-output video mapper -\033[0m" << endl;
    cout << "\t          \033[1m- Version " << PACKAGE_VERSION << " -\033[0m" << endl;
    cout << endl;

    // Get the executable directory
    string executable = argv[0];
    _executionPath = Utils::getPathFromExecutablePath(executable);

    // Parse the other args
    int idx = 1;
    string filename = string(DATADIR) + "splash.json";
    bool defaultFile = true;
    while (idx < argc)
    {
        if ((string(argv[idx]) == "-o" || string(argv[idx]) == "--open") && idx + 1 < argc)
        {
            defaultFile = false;
            filename = string(argv[idx + 1]);
            idx += 2;
        }
        else if (string(argv[idx]) == "-d" || string(argv[idx]) == "--debug")
        {
            Log::get().setVerbosity(Log::DEBUGGING);
            idx++;
        }
#if HAVE_LINUX
        else if (string(argv[idx]) == "-D" || string(argv[idx]) == "--forceDisplay")
        {
            auto regDisplayFull = regex("(:[0-9]\\.[0-9])", regex_constants::extended);
            auto regDisplayInt = regex("[0-9]", regex_constants::extended);
            smatch match;

            _forcedDisplay = string(argv[idx + 1]);
            if (regex_match(_forcedDisplay, match, regDisplayFull))
            {
                Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Display forced to " << _forcedDisplay << Log::endl;
            }
            else if (regex_match(_forcedDisplay, match, regDisplayInt))
            {
                Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Display forced to :0." << _forcedDisplay << Log::endl;
            }
            else
            {
                Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - " << string(argv[idx])
                           << ": argument expects a positive integer, or a string in the form of \":x.y\"" << Log::endl;
                exit(0);
            }

            idx += 2;
        }
#endif
        else if (string(argv[idx]) == "-t" || string(argv[idx]) == "--timer")
        {
            Timer::get().setDebug(true);
            idx++;
        }
        else if (string(argv[idx]) == "-s" || string(argv[idx]) == "--silent")
        {
            Log::get().setVerbosity(Log::NONE);
            idx++;
        }
        else if (string(argv[idx]) == "-h" || string(argv[idx]) == "--help")
        {
            cout << "Basic usage: splash [config.json]" << endl;
            cout << "Options:" << endl;
            cout << "\t-o (--open) [filename] : set [filename] as the configuration file to open" << endl;
            cout << "\t-d (--debug) : activate debug messages (if Splash was compiled with -DDEBUG)" << endl;
            cout << "\t-t (--timer) : activate more timers, at the cost of performance" << endl;
#if HAVE_LINUX
            cout << "\t-D (--forceDisplay) : force the display on which to show all windows" << endl;
#endif
            cout << "\t-s (--silent) : disable all messages" << endl;
            cout << "\t-i (--info) : get description for all objects attributes" << endl;
            cout << "\t-H (--hide) : run Splash in background" << endl;
            cout << "\t-l (--log2file) : write the logs to /var/log/splash.log, if possible" << endl;
            cout << endl;
            exit(0);
        }
        else if (string(argv[idx]) == "-H" || string(argv[idx]) == "--hide")
        {
            _runInBackground = true;
            idx++;
        }
        else if (string(argv[idx]) == "-i" || string(argv[idx]) == "--info")
        {
            auto descriptions = getObjectsAttributesDescriptions();
            cout << descriptions << endl;
            exit(0);
        }
        else if (string(argv[idx]) == "-l" || string(argv[idx]) == "--log2file")
        {
            // Called twice, once for the world, once to tell the Scenes to save log too
            setAttribute("logToFile", {1});
            addTask([&]() { setAttribute("logToFile", {1}); });
            idx++;
        }
        else if (defaultFile)
        {
            filename = string(argv[idx]);
            defaultFile = false;
            idx++;
        }
        else
        {
            idx++;
        }
    }

    if (defaultFile)
        Log::get() << Log::MESSAGE << "No filename specified, loading default file" << Log::endl;

    if (filename != "")
    {
        Json::Value config;
        _status &= loadConfig(filename, config);

        if (_status)
            _config = config;
        else
            exit(0);
    }
    else
        exit(0);
}

/*************/
void World::setAttribute(const string& name, const string& attrib, const Values& args)
{
    if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
}

/*************/
void World::registerAttributes()
{
    RootObject::registerAttributes();

    addAttribute("addObject",
        [&](const Values& args) {
            addTask([=]() {
                auto type = args[0].as<string>();
                auto name = string();

                if (args.size() == 1)
                    name = type + "_" + to_string(getId());
                else if (args.size() == 2)
                    name = args[1].as<string>();

                lock_guard<recursive_mutex> lockObjects(_objectsMutex);

                for (auto& s : _scenes)
                {
                    sendMessage(s.first, "add", {type, name});
                    addLocally(type, name, s.first);
                }

                auto path = Utils::getPathFromFilePath(_configFilename);
                set(name, "configFilePath", {path}, false);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("addObject", "Add an object to the scenes");

    addAttribute("sceneLaunched", [&](const Values& args) {
        lock_guard<mutex> lockChildProcess(_childProcessMutex);
        _sceneLaunched = true;
        _childProcessConditionVariable.notify_all();
        return true;
    });
    setAttributeDescription("sceneLaunched", "Message sent by Scenes to confirm they are running");

    addAttribute("deleteObject",
        [&](const Values& args) {
            addTask([=]() {
                lock_guard<recursive_mutex> lockObjects(_objectsMutex);
                auto objectName = args[0].as<string>();

                // Delete the object here
                auto objectDestIt = _objectDest.find(objectName);
                if (objectDestIt != _objectDest.end())
                    _objectDest.erase(objectDestIt);

                auto objectIt = _objects.find(objectName);
                if (objectIt != _objects.end())
                    _objects.erase(objectIt);

                // Ask for Scenes to delete the object
                sendMessage(SPLASH_ALL_PEERS, "deleteObject", args);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("deleteObject", "Delete an object given its name");

    addAttribute("flashBG",
        [&](const Values& args) {
            addTask([=]() { sendMessage(SPLASH_ALL_PEERS, "flashBG", {args[0].as<int>()}); });

            return true;
        },
        {'n'});
    setAttributeDescription("flashBG", "Switches the background color from black to light grey");

#if HAVE_LINUX
    addAttribute("forceCoreAffinity",
        [&](const Values& args) {
            _enforceCoreAffinity = args[0].as<int>();

            if (!_enforceCoreAffinity)
                return true;

            int sceneCount = _scenes.size();
            int sceneAffinity = 1;
            for (auto& s : _scenes)
            {
                sendMessage(s.first, "sceneAffinity", {sceneAffinity, sceneCount + 1});
                sceneAffinity++;
            }

            addTask([=]() {
                if (Utils::setAffinity({0}))
                    Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Set to run on the first CPU core" << Log::endl;
                else
                    Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to set World CPU core affinity" << Log::endl;

                vector<int> cores{};
                for (int i = sceneCount + 1; i < Utils::getCoreCount(); ++i)
                    cores.push_back(i);
                SThread::pool.setAffinity(cores);
            });

            return true;
        },
        [&]() -> Values { return {(int)_enforceCoreAffinity}; },
        {'n'});
    setAttributeDescription("forceCoreAffinity", "Enforce the World and Scene loops onto their own cores. The thread pool will use the remaining cores.");

    addAttribute("forceRealtime",
        [&](const Values& args) {
            _enforceRealtime = args[0].as<int>();

            if (!_enforceRealtime)
                return true;

            addTask([=]() {
                if (Utils::setRealTime())
                    Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Set to realtime priority" << Log::endl;
                else
                    Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to set scheduling priority" << Log::endl;
            });

            return true;
        },
        [&]() -> Values { return {(int)_enforceRealtime}; },
        {'n'});
    setAttributeDescription("forceRealtime", "Ask the scheduler to run Splash with realtime priority.");
#endif

    addAttribute("framerate",
        [&](const Values& args) {
            _worldFramerate = std::max(1, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {(int)_worldFramerate}; },
        {'n'});
    setAttributeDescription("framerate", "Set the refresh rate for the world (no relation to video framerate)");

    addAttribute("getAttribute",
        [&](const Values& args) {
            addTask([=]() {
                auto objectName = args[0].as<string>();
                auto attrName = args[1].as<string>();

                auto objectIt = _objects.find(objectName);
                if (objectIt != _objects.end())
                {
                    auto& object = objectIt->second;
                    Values values{};
                    object->getAttribute(attrName, values);

                    values.push_front("getAttribute");
                    sendMessage(SPLASH_ALL_PEERS, "answerMessage", values);
                }
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("getAttribute", "Ask the given object for the given attribute");

    addAttribute("getAttributeDescription",
        [&](const Values& args) {

            auto objectName = args[0].as<string>();
            auto attrName = args[1].as<string>();

            addTask([=]() {
                lock_guard<recursive_mutex> lock(_objectsMutex);

                auto objectIt = _objects.find(objectName);
                // If the object exists locally
                if (objectIt != _objects.end())
                {
                    auto& object = objectIt->second;
                    Values values{"getAttributeDescription"};
                    values.push_back(object->getAttributeDescription(attrName));
                    sendMessage(SPLASH_ALL_PEERS, "answerMessage", values);
                }
                // Else, ask the Scenes for some info
                else
                {
                    sendMessage(SPLASH_ALL_PEERS, "answerMessage", {""});
                }
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("getAttributeDescription", "Ask the given object for the description of the given attribute");

    addAttribute("getWorldAttribute",
        [&](const Values& args) {
            auto attrName = args[0].as<string>();
            addTask([=]() {
                Values attr;
                getAttribute(attrName, attr);
                attr.push_front("getWorldAttribute");
                sendMessage(SPLASH_ALL_PEERS, "answerMessage", attr);
            });
            return true;
        },
        {'s'});
    setAttributeDescription("getWorldAttribute", "Get a World's attribute and send it to the Scenes");

    addAttribute("loadConfig",
        [&](const Values& args) {
            string filename = args[0].as<string>();
            SThread::pool.enqueueWithoutId([=]() {
                Json::Value config;
                if (loadConfig(filename, config))
                {
                    for (auto& s : _scenes)
                    {
                        sendMessage(s.first, "quit", {});
                        _link->disconnectFrom(s.first);
                        if (s.second != -1)
                        {
                            waitpid(s.second, nullptr, 0);
                        }
                        else
                        {
                            if (_innerSceneThread.joinable())
                                _innerSceneThread.join();
                            _innerScene.reset();
                        }
                    }

                    _masterSceneName = "";

                    _config = config;
                    _reloadingConfig = true;
                    applyConfig();
                }
            });
            return true;
        },
        {'s'});
    setAttributeDescription("loadConfig", "Load the given configuration file");

    addAttribute("copyCameraParameters",
        [&](const Values& args) {
            string filename = args[0].as<string>();
            addTask([=]() { copyCameraParameters(filename); });
            return true;
        },
        {'s'});
    setAttributeDescription("copyCameraParameters", "Copy the camera parameters from the given configuration file (based on camera names)");

#if HAVE_PORTAUDIO
    addAttribute("clockDeviceName",
        [&](const Values& args) {
            addTask([=]() {
                auto clockDeviceName = args[0].as<string>();
                if (clockDeviceName != _clockDeviceName)
                {
                    _clockDeviceName = clockDeviceName;
                    _clock.reset();
                    _clock = unique_ptr<LtcClock>(new LtcClock(true, _clockDeviceName));
                }
            });

            return true;
        },
        [&]() -> Values { return {_clockDeviceName}; },
        {'s'});
    setAttributeDescription("clockDeviceName", "Set the audio device name from which to read the LTC clock signal");
#endif

    addAttribute("looseClock",
        [&](const Values& args) {
            Timer::get().setLoose(args[0].as<bool>());
            return true;
        },
        [&]() -> Values { return {static_cast<int>(Timer::get().isLoose())}; },
        {'n'});

    addAttribute("pong",
        [&](const Values& args) {
            Timer::get() >> "pingScene " + args[0].as<string>();
            return true;
        },
        {'s'});

    addAttribute("quit", [&](const Values& args) {
        _quit = true;
        return true;
    });
    setAttributeDescription("quit", "Ask the world to quit");

    addAttribute("renameObject",
        [&](const Values& args) {
            auto name = args[0].as<string>();
            auto newName = args[1].as<string>();

            addTask([=]() {
                lock_guard<recursive_mutex> lock(_objectsMutex);

                // Update the name in the World
                auto objIt = _objects.find(name);
                if (objIt != _objects.end())
                {
                    auto object = objIt->second;
                    object->setName(newName);
                    _objects[newName] = object;
                    _objects.erase(objIt);

                    auto objDestIt = _objectDest.find(name);
                    if (objDestIt != _objectDest.end())
                    {
                        _objectDest.erase(objDestIt);
                        _objectDest[newName] = {newName};
                    }
                }

                // Update the name in the Scenes
                for (const auto& scene : _scenes)
                    sendMessage(scene.first, "renameObject", {name, newName});
            });

            return true;
        },
        {'s', 's'});

    addAttribute("replaceObject",
        [&](const Values& args) {
            auto objName = args[0].as<string>();
            auto objType = args[1].as<string>();
            vector<string> targets;
            for (int i = 2; i < args.size(); ++i)
                targets.push_back(args[i].as<string>());

            if (!_factory->isCreatable(objType))
                return false;

            setAttribute("deleteObject", {objName});
            setAttribute("addObject", {objType, objName});
            addTask([=]() {
                for (const auto& t : targets)
                {
                    setAttribute("sendAllScenes", {"link", objName, t});
                    setAttribute("sendAllScenes", {"linkGhost", objName, t});
                }
            });
            return true;
        },
        {'s', 's'});
    setAttributeDescription("replaceObject", "Replace the given object by an object of the given type, and links the new object to the objects given by the following parameters");

    addAttribute("save", [&](const Values& args) {
        if (args.size() != 0)
            _configFilename = args[0].as<string>();

        addTask([=]() {
            Log::get() << "Saving configuration" << Log::endl;
            saveConfig();
        });
        return true;
    });
    setAttributeDescription("save", "Save the configuration to the current file (or a new one if a name is given as parameter)");

    addAttribute("saveProject",
        [&](const Values& args) {
            _projectFilename = args[0].as<string>();
            addTask([=]() {
                Log::get() << "Saving partial configuration to " << _projectFilename << Log::endl;
                saveProject();
            });
            return true;
        },
        {'s'});
    setAttributeDescription("saveProject", "Save only the configuration of images, textures and meshes");

    addAttribute("loadProject",
        [&](const Values& args) {
            _projectFilename = args[0].as<string>();
            addTask([=]() {
                Log::get() << "Loading partial configuration from " << _projectFilename << Log::endl;
                loadProject(_projectFilename);
            });
            return true;
        },
        {'s'});
    setAttributeDescription("loadProject", "Load only the configuration of images, textures and meshes");

    addAttribute("logToFile",
        [&](const Values& args) {
            Log::get().logToFile(args[0].as<bool>());
            setAttribute("sendAllScenes", {"logToFile", args[0]});
            return true;
        },
        {'n'});
    setAttributeDescription("logToFile", "If set to 1, the process holding the World will try to write log to file");

    addAttribute("sendAll",
        [&](const Values& args) {
            addTask([=]() {
                string name = args[0].as<string>();
                string attr = args[1].as<string>();
                auto values = args;

                // Ask for update of the ghost object if needed
                sendMessage(_masterSceneName, "setGhost", values);

                // Send the updated values to all scenes
                values.erase(values.begin());
                values.erase(values.begin());
                sendMessage(name, attr, values);

                // Also update local version
                if (_objects.find(name) != _objects.end())
                    _objects[name]->setAttribute(attr, values);
            });

            return true;
        },
        {'s', 's'});
    setAttributeDescription("sendAll", "Send to the given object in all Scenes the given message (all following arguments)");

    addAttribute("sendAllScenes",
        [&](const Values& args) {
            string attr = args[0].as<string>();
            Values values = args;
            values.erase(values.begin());
            for (auto& scene : _scenes)
                sendMessage(scene.first, attr, values);

            return true;
        },
        {'s'});
    setAttributeDescription("sendAllScenes", "Send the given message to all Scenes");

    addAttribute("sendToMasterScene",
        [&](const Values& args) {
            addTask([=]() {
                auto attr = args[0].as<string>();
                Values values = args;
                values.erase(values.begin());
                sendMessage(_masterSceneName, attr, values);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("sendToMasterScene", "Send the given message to the master Scene");

    addAttribute("pingTest",
        [&](const Values& args) {
            auto doPing = args[0].as<int>();
            if (doPing)
            {
                addRecurringTask("pingTest", [&]() {
                    static auto frameIndex = 0;
                    if (frameIndex == 0)
                    {
                        for (auto& scene : _scenes)
                        {
                            Timer::get() << "pingScene " + scene.first;
                            sendMessage(scene.first, "ping", {});
                        }
                    }
                    frameIndex = (frameIndex + 1) % 60;
                });
            }
            else
            {
                removeRecurringTask("pingTest");
            }

            return true;
        },
        {'n'});
    setAttributeDescription("pingTest", "Activate ping test if set to 1");

    addAttribute("swapTest",
        [&](const Values& args) {
            _swapSynchronizationTesting = args[0].as<int>();
            if (_swapSynchronizationTesting)
            {
                addRecurringTask("swapTest", [&]() {
                    sendMessage(SPLASH_ALL_PEERS, "swapTest", {1});
                    static auto frameNbr = 0;
                    static auto frameStatus = 0;
                    auto color = glm::vec4(0.0);

                    if (frameNbr == 0 && frameStatus == 0)
                    {
                        color = glm::vec4(0.0, 0.0, 0.0, 1.0);
                        frameStatus = 1;
                    }
                    else if (frameNbr == 0 && frameStatus == 1)
                    {
                        color = glm::vec4(1.0, 1.0, 1.0, 1.0);
                        frameStatus = 0;
                    }

                    if (frameNbr == 0)
                        sendMessage(SPLASH_ALL_PEERS, "swapTestColor", {color[0], color[1], color[2], color[3]});

                    frameNbr = (frameNbr + 1) % _swapSynchronizationTesting;
                });
            }
            else
            {
                removeRecurringTask("swapTest");
                addTask([&]() { sendMessage(SPLASH_ALL_PEERS, "swapTest", {0}); });
            }
            return true;
        },
        {'n'});
    setAttributeDescription("swapTest", "Activate video swap test if set to 1");

    addAttribute("wireframe",
        [&](const Values& args) {
            addTask([=]() { sendMessage(SPLASH_ALL_PEERS, "wireframe", {args[0].as<int>()}); });

            return true;
        },
        {'n'});
    setAttributeDescription("wireframe", "Show all meshes as wireframes if set to 1");

    addAttribute("configurationPath",
        [&](const Values& args) {
            _configurationPath = args[0].as<string>();
            addTask([=]() { sendMessage(SPLASH_ALL_PEERS, "configurationPath", {_configurationPath}); });
            return true;
        },
        [&]() -> Values { return {_configurationPath}; },
        {'s'});
    setAttributeDescription("configurationPath", "Path to the configuration files");

    addAttribute("mediaPath",
        [&](const Values& args) {
            _mediaPath = args[0].as<string>();
            addTask([=]() { sendMessage(SPLASH_ALL_PEERS, "mediaPath", {_mediaPath}); });
            return true;
        },
        [&]() -> Values { return {_mediaPath}; },
        {'s'});
    setAttributeDescription("mediaPath", "Path to the media files");
}
}
