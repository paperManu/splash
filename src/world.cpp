#include "world.h"

#include <chrono>
#include <fstream>
#include <unistd.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spawn.h>
#include <sys/wait.h>

#include "./image.h"
#if HAVE_GPHOTO
    #include "./image_gphoto.h"
#endif
#if HAVE_FFMPEG
    #include "./image_ffmpeg.h"
#endif
#if HAVE_OPENCV
    #include "./image_opencv.h"
#endif
#if HAVE_SHMDATA
    #include "./image_shmdata.h"
    #include "./mesh_shmdata.h"
#endif
#include "./link.h"
#include "./log.h"
#include "./mesh.h"
#include "./osUtils.h"
#include "./queue.h"
#include "./scene.h"
#include "./timer.h"
#include "./threadpool.h"

// Included only for creating the documentation through the --info flag
#include "./geometry.h"
#include "./window.h"

using namespace glm;
using namespace std;

namespace Splash {
/*************/
World* World::_that;

/*************/
World::World(int argc, char** argv)
{
    parseArguments(argc, argv);

    init();
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

    // We must not send the timings too often, this is what this variable is for
    int frameIndice {0};

    while (true)
    {
        Timer::get() << "worldLoop";
        lock_guard<mutex> lockConfiguration(_configurationMutex);

        {
            // Execute waiting tasks
            lock_guard<mutex> lockTask(_taskMutex);
            for (auto& task : _taskQueue)
                task();
            _taskQueue.clear();
        }

        {
            lock_guard<recursive_mutex> lockObjects(_objectsMutex);

            // Read and serialize new buffers
            Timer::get() << "serialize";
            vector<unsigned int> threadIds;
            map<string, shared_ptr<SerializedObject>> serializedObjects;
            for (auto& o : _objects)
            {
                auto bufferObj = dynamic_pointer_cast<BufferObject>(o.second);
                // This prevents the map structure to be modified in the threads
                serializedObjects.emplace(std::make_pair(bufferObj->getDistantName(), make_shared<SerializedObject>()));

                threadIds.push_back(SThread::pool.enqueue([=, &serializedObjects, &o]() {
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
                                serializedObjects[bufferObj->getDistantName()] = obj;
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

        // If swap synchronization test is enabled
        if (_swapSynchronizationTesting)
        {
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
        }
        else
        {
            sendMessage(SPLASH_ALL_PEERS, "swapTest", {0});
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

        // Ping the clients once in a while
        if (Timer::get().isDebug())
        {
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
        }

        // Get the current FPS
        Timer::get() >> 1e6 / (float)_worldFramerate >> "worldLoop";
    }
}

/*************/
void World::addLocally(string type, string name, string destination)
{
    // Images and Meshes have a counterpart on this side
    if (type.find("image") == string::npos && 
        type.find("mesh") == string::npos && 
        type.find("queue") == string::npos)
        return;

    auto object = _factory->create(type);
    if (object.get() != nullptr)
    {
        object->setId(getId());
        name = object->setName(name); // The real name is not necessarily the one we set (see Queues)
        _objects[name] = object;
    }

    // If the object is not registered yet, we add it with the specified destination
    // as well as the WORLD_SCENE destination
    if (_objectDest.find(name) == _objectDest.end())
    {
        _objectDest[name] = vector<string>();
        _objectDest[name].emplace_back(destination);
    }
    // If it is, we only add the new destination
    else
    {
        bool isPresent = false;
        for (auto d : _objectDest[name])
            if (d == destination)
                isPresent = true;
        if (!isPresent)
        {
            _objectDest[name].emplace_back(destination);
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

    // Get the list of all scenes, and create them
    const Json::Value jsScenes = _config["scenes"];
    for (int i = 0; i < jsScenes.size(); ++i)
    {
        if (jsScenes[i]["address"].asString() == string("localhost"))
        {
            if (!jsScenes[i].isMember("name"))
            {
                Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Scenes need a name" << Log::endl;
                return;
            }
            int spawn = 0;
            if (jsScenes[i].isMember("spawn"))
                spawn = jsScenes[i]["spawn"].asInt();

            string display = "DISPLAY=:0.";
#if HAVE_LINUX
            if (jsScenes[i].isMember("display"))
                display += to_string(jsScenes[i]["display"].asInt());
            else
                display += to_string(0);
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
                    _innerScene = make_shared<Scene>(name, false);
                    _innerSceneThread = thread([&]() {
                        _innerScene->run();
                    });
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
                        Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Timeout when trying to connect to newly spawned scene \"" << name << "\". Exiting." << Log::endl;
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
                _link->connectTo(name, _innerScene);
            else
                _link->connectTo(name);

            // Set the remaining parameters
            auto sceneMembers = jsScenes[i].getMemberNames();
            int idx {0};
            for (const auto& param : jsScenes[i])
            {
                string paramName = sceneMembers[idx];

                Values values;
                if (param.isArray())
                    values = processArray(param);
                else if (param.isInt())
                    values.emplace_back(param.asInt());
                else if (param.isDouble())
                    values.emplace_back(param.asFloat());
                else if (param.isString())
                    values.emplace_back(param.asString());

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
        int idx {0};
        for (const auto& obj : jsScene)
        {
            string name = sceneMembers[idx];
            if (name == "links" || !obj.isMember("type"))
            {
                idx++;
                continue;
            }

            string type = obj["type"].asString();

            // Before anything, all objects have the right to know what the current path is
            if (type != "scene")
            {
                auto path = Utils::getPathFromFilePath(_configFilename);
                sendMessage(name, "configFilePath", {path});
                if (s.first != _masterSceneName)
                    sendMessage(_masterSceneName, "setGhost", {name, "configFilePath", path});
                set(name, "configFilePath", {path}, false);
            }

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

                Values values;
                if (attr.isArray())
                    values = processArray(attr);
                else if (attr.isInt())
                    values.emplace_back(attr.asInt());
                else if (attr.isDouble())
                    values.emplace_back(attr.asFloat());
                else if (attr.isString())
                    values.emplace_back(attr.asString());

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
        int idx {0};
        for (const auto& attr : jsWorld)
        {
            Values values;
            if (attr.isArray())
                values = processArray(attr);
            else if (attr.isInt())
                values.emplace_back(attr.asInt());
            else if (attr.isDouble())
                values.emplace_back(attr.asFloat());
            else if (attr.isString())
                values.emplace_back(attr.asString());

            string paramName = worldMember[idx];
            setAttribute(paramName, values);
            idx++;
        }
    }

    // Also, enable the master clock if it was not enabled
#if HAVE_PORTAUDIO
    if (!_clock)
        _clock = unique_ptr<LtcClock>(new LtcClock(true));
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

    // We create "fake" objects and ask then for their attributes
    auto localFactory = Factory();
    auto types = localFactory.getObjectTypes();
    for (auto& type : types)
    {
        auto obj = localFactory.create(type);
        auto description = obj->getAttributesDescriptions();

        int addedAttribute = 0;
        root[obj->getType()] = Json::Value();
        for (auto& d : description)
        {
            // We only keep attributes with a valid documentation
            // The other ones are inner attributes
            if (d[1].asString().size() == 0)
                continue;

            // We also don't keep attributes with no argument types
            if (d[2].asValues().size() == 0)
                continue;

            string descriptionStr = "[";
            auto type = d[2].asValues();
            for (int i = 0; i < type.size(); ++i)
            {
                descriptionStr += type[i].asString();
                if (i < type.size() - 1)
                    descriptionStr += ", ";
            }
            descriptionStr += "] " + d[1].asString();
                
            root[obj->getType()][d[0].asString()] = descriptionStr;
            
            addedAttribute++;
        }

        // If the object has no documented attribute
        if (addedAttribute == 0)
            root.removeMember(obj->getType());
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
        reader.parse(answer[2].asString(), config);
        root[s.first] = config;
    }

    // Local objects configuration can differ from the scenes objects, 
    // as their type is not necessarily identical
    Json::Value& jsScenes = _config["scenes"];
    for (int i = 0; i < jsScenes.size(); ++i)
    {
        string sceneName = jsScenes[i]["name"].asString();

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
Values World::getObjectsNameByType(string type)
{
    Values answer = sendMessageWithAnswer(_masterSceneName, "getObjectsNameByType", {type});
    return answer[2].asValues();
}

/*************/
void World::handleSerializedObject(const string name, shared_ptr<SerializedObject> obj)
{
    _link->sendBuffer(name, std::move(obj));
}

/*************/
void World::init()
{
    _self = shared_ptr<World>(this, [](World*){}); // A shared pointer with no deleter, how convenient

    _type = "world";
    _name = "world";

    _that = this;
    _signals.sa_handler = leave;
    _signals.sa_flags = 0;
    sigaction(SIGINT, &_signals, NULL);
    sigaction(SIGTERM, &_signals, NULL);

    _link = make_shared<Link>(weak_ptr<World>(_self), _name);
    _factory = unique_ptr<Factory>(new Factory(_self));

    registerAttributes();
}

/*************/
void World::leave(int signal_value)
{
    Log::get() << "World::" << __FUNCTION__ << " - Received a SIG event. Quitting." << Log::endl;
    _that->_quit = true;
}

/*************/
bool World::copyCameraParameters(std::string filename)
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

                Values values;
                if (attr.isArray())
                    values = processArray(attr);
                else if (attr.isInt())
                    values.emplace_back(attr.asInt());
                else if (attr.isDouble())
                    values.emplace_back(attr.asFloat());
                else if (attr.isString())
                    values.emplace_back(attr.asString());

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
Values World::processArray(Json::Value values)
{
    Values outValues;
    for (const auto& v : values)
    {
        if (v.isInt())
            outValues.emplace_back(v.asInt());
        else if (v.isDouble())
            outValues.emplace_back(v.asFloat());
        else if (v.isArray())
            outValues.emplace_back(processArray(v));
        else
            outValues.emplace_back(v.asString());
    }
    return outValues;
}

/*************/
bool World::loadConfig(string filename, Json::Value& configuration)
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

    _configFilename = filename;
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
            cout << "\t-s (--silent) : disable all messages" << endl;
            cout << "\t-i (--info) : get description for all objects attributes" << endl;
            cout << endl;
            exit(0);
        }
        else if (string(argv[idx]) == "-i" || string(argv[idx]) == "--info")
        {
            auto descriptions = getObjectsAttributesDescriptions();
            cout << descriptions << endl;
            exit(0);
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
void World::setAttribute(string name, string attrib, const Values& args)
{
    if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
}

/*************/
void World::registerAttributes()
{
    addAttribute("addObject", [&](const Values& args) {
        addTask([=]() {
            auto type = args[0].asString();
            auto name = string();

            if (args.size() == 1)
                name = type + "_" + to_string(getId());
            else if (args.size() == 2)
                name = args[1].asString();

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
    }, {'s'});
    setAttributeDescription("addObject", "Add an object to the scenes");

    addAttribute("sceneLaunched", [&](const Values& args) {
        lock_guard<mutex> lockChildProcess(_childProcessMutex);
        _sceneLaunched = true;
        _childProcessConditionVariable.notify_all();
        return true;
    });
    setAttributeDescription("sceneLaunched", "Message sent by Scenes to confirm they are running");

    addAttribute("computeBlending", [&](const Values& args) {
        _blendingMode = args[0].asString();
        sendMessage(SPLASH_ALL_PEERS, "computeBlending", {_blendingMode});

        return true;
    }, [&]() -> Values {
        return {_blendingMode};
    }, {'s'});
    setAttributeDescription("computeBlending", "Ask all Scenes to compute the blending");

    addAttribute("deleteObject", [&](const Values& args) {
        addTask([=]() {
            lock_guard<recursive_mutex> lockObjects(_objectsMutex);
            auto objectName = args[0].asString();

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
    }, {'s'});
    setAttributeDescription("deleteObject", "Delete an object given its name");

    addAttribute("flashBG", [&](const Values& args) {
        addTask([=]() {
            sendMessage(SPLASH_ALL_PEERS, "flashBG", {args[0].asInt()});
        });

        return true;
    }, {'n'});
    setAttributeDescription("flashBG", "Switches the background color from black to light grey");

    addAttribute("framerate", [&](const Values& args) {
        _worldFramerate = std::max(1, args[0].asInt());
        return true;
    }, [&]() -> Values {
        return {(int)_worldFramerate};
    }, {'n'});
    setAttributeDescription("framerate", "Set the refresh rate for the world (no relation to video framerate)");

    addAttribute("getAttribute", [&](const Values& args) {
        addTask([=]() {
            auto objectName = args[0].asString();
            auto attrName = args[1].asString();

            auto objectIt = _objects.find(objectName);
            if (objectIt != _objects.end())
            {
                auto& object = objectIt->second;
                Values values {};
                object->getAttribute(attrName, values);

                values.push_front("getAttribute");
                sendMessage(SPLASH_ALL_PEERS, "answerMessage", values);
            }
        });

        return true;
    }, {'s', 's'});
    setAttributeDescription("getAttribute", "Ask the given object for the given attribute");

    addAttribute("getAttributeDescription", [&](const Values& args) {
        lock_guard<recursive_mutex> lock(_objectsMutex);

        auto objectName = args[0].asString();
        auto attrName = args[1].asString();

        auto objectIt = _objects.find(objectName);
        // If the object exists locally
        if (objectIt != _objects.end())
        {
            auto& object = objectIt->second;
            Values values {"getAttributeDescription"};
            values.push_back(object->getAttributeDescription(attrName));
            sendMessage(SPLASH_ALL_PEERS, "answerMessage", values);
        }
        // Else, ask the Scenes for some info
        else
        {
            sendMessage(SPLASH_ALL_PEERS, "answerMessage", {""});
        }

        return true;
    }, {'s', 's'});
    setAttributeDescription("getAttributeDescription", "Ask the given object for the description of the given attribute");

    addAttribute("loadConfig", [&](const Values& args) {
        string filename = args[0].asString();
        SThread::pool.enqueueWithoutId([=]() {
            Json::Value config;
            if (loadConfig(filename, config))
            {
                for (auto& s : _scenes)
                {
                    sendMessage(s.first, "quit", {});
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

                    _link->disconnectFrom(s.first);
                }

                _masterSceneName = "";

                _config = config;
                _reloadingConfig = true;
                applyConfig();
            }
        });
        return true;
    }, {'s'});
    setAttributeDescription("loadConfig", "Load the given configuration file");

    addAttribute("copyCameraParameters", [&](const Values& args) {
        string filename = args[0].asString();
        addTask([=]() {
            copyCameraParameters(filename);
        });
        return true;
    }, {'s'});
    setAttributeDescription("copyCameraParameters", "Copy the camera parameters from the given configuration file (based on camera names)");

#if HAVE_PORTAUDIO
    addAttribute("clockDeviceName", [&](const Values& args) {
        addTask([=]() {
            _clockDeviceName = args[0].asString();
            if (_clockDeviceName != "")
                _clock = unique_ptr<LtcClock>(new LtcClock(true, _clockDeviceName));
            else if (_clock)
                _clock.reset();
        });

        return true;
    }, [&]() -> Values {
        return {_clockDeviceName};
    }, {'s'});
    setAttributeDescription("clockDeviceName", "Set the audio device name from which to read the LTC clock signal");
#endif

    addAttribute("pong", [&](const Values& args) {
        Timer::get() >> "pingScene " + args[0].asString();
        return true;
    }, {'s'});
    setAttributeDescription("pong", "Answer to ping");

    addAttribute("quit", [&](const Values& args) {
        _quit = true;
        return true;
    });
    setAttributeDescription("quit", "Ask the world to quit");

    addAttribute("renameObject", [&](const Values& args) {
        auto name = args[0].asString();
        auto newName = args[1].asString();

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
    }, {'s', 's'});

    addAttribute("replaceObject", [&](const Values& args) {
        auto objName = args[0].asString();
        auto objType = args[1].asString();
        vector<string> targets;
        for (int i = 2; i < args.size(); ++i)
            targets.push_back(args[i].asString());

        setAttribute("deleteObject", {objName});
        setAttribute("addObject", {objType, objName});
        addTask([=]() {
            for (const auto& t : targets)
            {
                setAttribute("sendAllScenes", {"link", objName, t});
                setAttribute("sendAllScenes", {"linkGhots", objName, t});
            }
        });
        return true;
    }, {'s', 's'});
    setAttributeDescription("replaceObject", "Replace the given object by an object of the given type");

    addAttribute("save", [&](const Values& args) {
        if (args.size() != 0)
            _configFilename = args[0].asString();

        addTask([=]() {
            Log::get() << "Saving configuration" << Log::endl;
            saveConfig();
        });
        return true;
    });
    setAttributeDescription("save", "Save the configuration to the current file (or a new one if a name is given as parameter)");

    addAttribute("sendAll", [&](const Values& args) {
        string name = args[0].asString();
        string attr = args[1].asString();
        Values values {name, attr};
        for (int i = 2; i < args.size(); ++i)
            values.push_back(args[i]);

        // Ask for update of the ghost object if needed
        sendMessage(_masterSceneName, "setGhost", values);
        
        // Send the updated values to all scenes
        values.erase(values.begin());
        values.erase(values.begin());
        sendMessage(name, attr, values);

        addTask([=]() {
            // Also update local version
            if (_objects.find(name) != _objects.end())
                _objects[name]->setAttribute(attr, values);
        });

        return true;
    }, {'s', 's'});
    setAttributeDescription("sendAll", "Send to the given object in all Scenes the given message (all following arguments)");

    addAttribute("sendAllScenes", [&](const Values& args) {
        string attr = args[0].asString();
        Values values = args;
        values.erase(values.begin());
        for (auto& scene : _scenes)
            sendMessage(scene.first, attr, values);

        return true;
    }, {'s'});
    setAttributeDescription("sendAllScenes", "Send the given message to all Scenes");

    addAttribute("sendToMasterScene", [&](const Values& args) {
        addTask([=]() {
            auto attr = args[0].asString();
            Values values = args;
            values.erase(values.begin());
            sendMessage(_masterSceneName, attr, values);
        });

        return true;
    }, {'s'});
    setAttributeDescription("sendToMasterScene", "Send the given message to the master Scene");

    addAttribute("swapTest", [&](const Values& args) {
        addTask([=]() {
            _swapSynchronizationTesting = args[0].asInt();
        });

        return true;
    }, {'n'});
    setAttributeDescription("swapTest", "Activate video swap test if set to 1");

    addAttribute("wireframe", [&](const Values& args) {
        addTask([=]() {
            sendMessage(SPLASH_ALL_PEERS, "wireframe", {args[0].asInt()});
        });

        return true;
    }, {'n'});
    setAttributeDescription("wireframe", "Show all meshes as wireframes if set to 1");
}

}
