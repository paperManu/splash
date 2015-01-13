#include "world.h"
#include "timer.h"

#include "image.h"
#if HAVE_GPHOTO
#include "image_gphoto.h"
#endif
#include "image_shmdata.h"
#include "link.h"
#include "log.h"
#include "mesh.h"
#include "mesh_shmdata.h"
#include "scene.h"
#include "threadpool.h"

#include <chrono>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <json/reader.h>
#include <json/writer.h>
#include <spawn.h>

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
    SLog::log << Log::DEBUGGING << "World::~World - Destructor" << Log::endl;
#endif
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
        STimer::timer << "worldLoop";

        STimer::timer << "upload";
        vector<unsigned int> threadIds;
        for (auto& o : _objects)
        {
            threadIds.push_back(SThread::pool.enqueue([=, &o]() {
                // Update the local objects
                o.second->update();

                // Send them the their destinations
                SerializedObjectPtr obj;
                BufferObjectPtr bufferObj = dynamic_pointer_cast<BufferObject>(o.second);
                if (bufferObj.get() != nullptr)
                {
                    if (bufferObj->wasUpdated()) // if the buffer has been updated
                    {
                        obj = bufferObj->serialize();
                        bufferObj->setNotUpdated();
                        _link->sendBuffer(o.first, obj);
                    }
                    else
                        return; // if not, exit this thread
                }
            }));
        }
        SThread::pool.waitThreads(threadIds);
        STimer::timer >> "upload";

        // Send current timings to all Scenes, for display purpose
        auto durationMap = STimer::timer.getDurationMap();
        for (auto& d : durationMap)
            sendMessage(SPLASH_ALL_PAIRS, "duration", {d.first, (int)d.second});

        // Send newer logs to all Scenes
        auto logs = SLog::log.getLogs();
        for (auto& log : logs)
            sendMessage(SPLASH_ALL_PAIRS, "log", {log.first, (int)log.second});

        if (_doComputeBlending)
        {
            sendMessage(SPLASH_ALL_PAIRS, "computeBlending", {});
            _doComputeBlending = false;
        }

        if (_doSaveConfig)
        {
            saveConfig();
            _doSaveConfig = false;
        }

        if (_quit)
        {
            for (auto& s : _scenes)
                sendMessage(s.first, "quit", {});
            break;
        }

        // Get the current FPS
        STimer::timer >> 1e6 / (float)_worldFramerate >> "worldLoop";
    }
}

/*************/
void World::addLocally(string type, string name, string destination)
{
    // Images and Meshes have a counterpart on this side
    if (type.find("image") == string::npos && type.find("mesh") == string::npos)
        return;

    BaseObjectPtr object;
    if (_objects.find(name) == _objects.end())
    {
        if (type == string("image"))
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image>());
        else if (type == string("image_shmdata"))
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image_Shmdata>());
#if HAVE_GPHOTO
        else if (type == string("image_gphoto"))
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image_GPhoto>());
#endif
        else if (type == string("mesh"))
            object = dynamic_pointer_cast<BaseObject>(make_shared<Mesh>());
        else if (type == string("mesh_shmdata"))
            object = dynamic_pointer_cast<BaseObject>(make_shared<Mesh_Shmdata>());
    }
    if (object.get() != nullptr)
    {
        object->setId(getId());
        object->setName(name);
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
    // We first destroy all scenes
    if (_scenes.size() > 0)
        _scenes.clear();

    // Configure this very World
    if (_config.isMember("world"))
    {
        const Json::Value jsWorld = _config["world"];
        auto worldMember = jsWorld.getMemberNames();
        int idx {0};
        for (const auto& param : jsWorld)
        {
            string paramName = worldMember[idx];
            Value v;
            if (param.isInt())
                v = param.asInt();
            else if (param.isDouble())
                v = param.asFloat();
            else
                v = param.asString();
            setAttribute(paramName, {v});
            idx++;
        }
    }

    // Get the list of all scenes, and create them
    const Json::Value jsScenes = _config["scenes"];
    for (int i = 0; i < jsScenes.size(); ++i)
    {
        if (jsScenes[i]["address"].asString() == string("localhost"))
        {
            if (!jsScenes[i].isMember("name"))
            {
                SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Scenes need a name" << Log::endl;
                return;
            }
            int spawn = 0;
            if (jsScenes[i].isMember("spawn"))
                spawn = jsScenes[i]["spawn"].asInt();

            string display = "DISPLAY=:0.";
            if (jsScenes[i].isMember("display"))
                display += to_string(jsScenes[i]["display"].asInt());
            else
                display += to_string(0);

            string name = jsScenes[i]["name"].asString();
            ScenePtr scene;
            if (spawn > 0)
            {
                // Spawn a new process containing this Scene
                _childProcessLaunched = false;
                int pid;

                string cmd = string(SPLASHPREFIX) + "/bin/splash-scene";
                string debug = (SLog::log.getVerbosity() == Log::DEBUGGING) ? "-d" : "";

                char* argv[] = {(char*)cmd.c_str(), (char*)debug.c_str(), (char*)name.c_str(), NULL};
                char* env[] = {(char*)display.c_str(), NULL};
                int status = posix_spawn(&pid, cmd.c_str(), NULL, NULL, argv, env);
                if (status != 0)
                    SLog::log << Log::ERROR << "World::" << __FUNCTION__ << " - Error while spawning process for scene " << name << Log::endl;

                // We wait for the child process to be launched
                timespec nap;
                nap.tv_sec = 1;
                nap.tv_nsec = 0;
                while (!_childProcessLaunched)
                    nanosleep(&nap, NULL);
            }
            _scenes[name] = scene;
            if (_masterSceneName == "")
                _masterSceneName = name;
            
            // Initialize the communication
            _link->connectTo(name);

            // Set the remaining parameters
            auto sceneMembers = jsScenes[i].getMemberNames();
            int idx {0};
            for (const auto& param : jsScenes[i])
            {
                string paramName = sceneMembers[idx];
                Value v;
                if (param.isInt())
                    v = param.asInt();
                else if (param.isDouble())
                    v = param.asFloat();
                else
                    v = param.asString();
                sendMessage(name, paramName, {v});
                idx++;
            }
        }
        else
        {
            SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Non-local scenes are not implemented yet" << Log::endl;
        }
    }

    // Configure each scenes
    // The first scene is the master one, and also receives some ghost objects
    // Currently, only cameras are concerned
    for (auto& s : _scenes)
    {
        if (!_config.isMember(s.first))
            continue;

        const Json::Value jsScene = _config[s.first];

        // Set if master
        if (s.first == _masterSceneName)
            sendMessage(_masterSceneName, "setMaster", {});

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
            if (type != "scene")
            {
                sendMessage(s.first, "add", {type, name});
                if (s.first != _masterSceneName)
                    sendMessage(_masterSceneName, "addGhost", {type, name});

                // Some objects are also created on this side, and linked with the distant one
                addLocally(type, name, s.first);
            }

            // Set their attributes
            auto objMembers = obj.getMemberNames();
            int idxAttr {0};
            for (const auto& attr : obj)
            {
                if (objMembers[idxAttr] == "type")
                {
                    idxAttr++;
                    continue;
                }

                // Helper function to read arrays
                std::function<Values(Json::Value)> processArray;
                processArray = [&processArray](Json::Value values) {
                    Values outValues;
                    for (auto& v : values)
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
                };

                Values values;
                if (attr.isArray())
                    values = processArray(attr);
                else if (attr.isInt())
                    values.emplace_back(attr.asInt());
                else if (attr.isDouble())
                    values.emplace_back(attr.asFloat());
                else if (attr.isString())
                    values.emplace_back(attr.asString());

                sendMessage(name, objMembers[idxAttr], values);
                if (type != "scene")
                {
                    if (s.first != _masterSceneName)
                    {
                        Values ghostValues {name, objMembers[idxAttr]};
                        for (auto& v : values)
                            ghostValues.push_back(v);
                        sendMessage(_masterSceneName, "setGhost", ghostValues);
                    }
                    // We also set the attribute locally, if the object exists
                    set(name, objMembers[idxAttr], values);
                }

                idxAttr++;
            }
            idx++;
        }

        // Link the objects together
        idx = 0;
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

    // Send the start message for all scenes
    sendMessage(SPLASH_ALL_PAIRS, "start", {});
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

    // Complete with the configuration from the world
    const Json::Value jsScenes = _config["scenes"];
    for (int i = 0; i < jsScenes.size(); ++i)
    {
        string sceneName = jsScenes[i]["name"].asString();

        if (root.isMember(sceneName))
        {
            Json::Value& scene = root[sceneName];
            Json::Value::Members members = scene.getMemberNames();

            for (auto& m : members)
            {
                if (!_config[sceneName].isMember(m))
                    _config[sceneName][m] = Json::Value();

                Json::Value::Members attributes = scene[m].getMemberNames();
                for (auto& a : attributes)
                    _config[sceneName][m][a] = scene[m][a];
            }
        }
    }
    
    setlocale(LC_NUMERIC, "C"); // Needed to make sure numbers are written with commas
    ofstream out(_configFilename, ios::binary);
    out << _config.toStyledString();
    out.close();
}

/*************/
void World::handleSerializedObject(const string name, const SerializedObjectPtr obj)
{
    _link->sendBuffer(name, obj);
}

/*************/
void World::init()
{
    _self = WorldPtr(this, [](World*){}); // A shared pointer with no deleter, how convenient

    _type = "World";
    _name = "world";

    _that = this;
    _signals.sa_handler = leave;
    _signals.sa_flags = 0;
    sigaction(SIGINT, &_signals, NULL);
    sigaction(SIGTERM, &_signals, NULL);

    _link = make_shared<Link>(weak_ptr<World>(_self), _name);

    registerAttributes();
}

/*************/
void World::leave(int signal_value)
{
    SLog::log << "World::" << __FUNCTION__ << " - Received a SIG event. Quitting." << Log::endl;
    _that->_quit = true;
}

/*************/
bool World::loadConfig(string filename)
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
        SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to open file " << filename << Log::endl;
        return false;
    }

    _configFilename = filename;
    Json::Value config;
    Json::Reader reader;

    bool success = reader.parse(contents, config);
    if (!success)
    {
        SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to parse file " << filename << Log::endl;
        SLog::log << Log::WARNING << reader.getFormattedErrorMessages() << Log::endl;
        return false;
    }

    _config = config;
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

    int idx = 0;
    string filename {""};
    while (idx < argc)
    {
        if ((string(argv[idx]) == "-o" || string(argv[idx]) == "--open") && idx + 1 < argc)
        {
            filename = string(argv[idx + 1]);
            idx += 2;
        }
        else if (string(argv[idx]) == "-d" || string(argv[idx]) == "--debug")
        {
#ifdef DEBUG
            SLog::log.setVerbosity(Log::DEBUGGING);
#endif
            idx++;
        }
        else if (string(argv[idx]) == "-s" || string(argv[idx]) == "--silent")
        {
            SLog::log.setVerbosity(Log::NONE);
            idx++;
        }
        else if (string(argv[idx]) == "-h" || string(argv[idx]) == "--help")
        {
            cout << "Basic usage: splash -o [config.json]" << endl;
            cout << "Options:" << endl;
            cout << "\t-o (--open) [filename] : set [filename] as the configuration file to open" << endl;
            cout << "\t-d (--debug) : activate debug messages (if Splash was compiled with -DDEBUG)" << endl;
            cout << "\t-s (--silent) : disable all messages" << endl;
            exit(0);
        }
        else
            idx++;
    }

    if (filename != "")
        _status &= loadConfig(filename);
    else
        exit(0);
}

/*************/
void World::setAttribute(string name, string attrib, Values args)
{
    if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
}

/*************/
void World::registerAttributes()
{
    _attribFunctions["childProcessLaunched"] = AttributeFunctor([&](Values args) {
        _childProcessLaunched = true;
        return true;
    });

    _attribFunctions["computeBlending"] = AttributeFunctor([&](Values args) {
        if (args.size() == 0 || args[0].asInt() != 0)
            _doComputeBlending = true;
        return true;
    });

    _attribFunctions["flashBG"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        sendMessage(SPLASH_ALL_PAIRS, "flashBG", {args[0].asInt()});
        return true;
    });

    _attribFunctions["framerate"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        _worldFramerate = std::max(1, args[0].asInt());
        return true;
    });

    _attribFunctions["quit"] = AttributeFunctor([&](Values args) {
        _quit = true;
        return true;
    });

    _attribFunctions["save"] = AttributeFunctor([&](Values args) {
        SLog::log << "Saving configuration" << Log::endl;
        _doSaveConfig = true;
        return true;
    });

    _attribFunctions["sendAll"] = AttributeFunctor([&](Values args) {
        if (args.size() < 2)
            return false;
        string name = args[0].asString();
        string attr = args[1].asString();
        Values values {name, attr};
        for (int i = 2; i < args.size(); ++i)
            values.push_back(args[i]);

        sendMessage(_masterSceneName, "setGhost", values);
        
        values.erase(values.begin());
        values.erase(values.begin());
        sendMessage(name, attr, values);
        return true;
    });

    _attribFunctions["wireframe"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        sendMessage(SPLASH_ALL_PAIRS, "wireframe", {args[0].asInt()});
        return true;
    });
}

}
