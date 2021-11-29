#include "./core/world.h"

#include <chrono>
#include <fstream>
#include <regex>
#include <set>
#include <utility>

#include <getopt.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glm/gtc/matrix_transform.hpp>
#include <Tracy.hpp>

#include "./core/buffer_object.h"
#include "./core/constants.h"
#include "./network/link.h"
#include "./core/scene.h"
#include "./core/serializer.h"
#include "./image/image.h"
#include "./image/queue.h"
#include "./mesh/mesh.h"
#include "./utils/jsonutils.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

using namespace glm;

namespace Splash
{
/*************/
World* World::_that;

/*************/
World::World(Context context)
    : RootObject(context)
{
    _name = "world";

    _that = this;
    _signals.sa_handler = leave;
    _signals.sa_flags = 0;
    sigaction(SIGINT, &_signals, nullptr);
    sigaction(SIGTERM, &_signals, nullptr);

    registerAttributes();
    initializeTree();
}

/*************/
bool World::applyContext()
{
    if (_context.info)
    {
        auto descriptions = getObjectsAttributesDescriptions();
        std::cout << descriptions << "\n";
        return false;
    }

    if (_context.pythonScriptPath)
    {
        auto pythonScriptPath = *_context.pythonScriptPath;
        auto pythonArgs = _context.pythonArgs;

        // The Python script will be added once the loop runs
        addTask([=]() {
            Log::get() << Log::MESSAGE
                       << "World::parseArguments - Adding Python script from command "
                          "line argument: "
                       << pythonScriptPath << Log::endl;

            auto pythonObjectName = std::string("_pythonArgScript");
            if (!_nameRegistry.registerName(pythonObjectName))
                pythonObjectName = _nameRegistry.generateName("_pythonArgScript");

            sendMessage(Constants::ALL_PEERS, "addObject", {"python", pythonObjectName, _masterSceneName});
            sendMessage(pythonObjectName, "savable", {false});
            sendMessage(pythonObjectName, "args", {pythonArgs});
            sendMessage(pythonObjectName, "file", {pythonScriptPath});
        });
    }

    if (_context.socketPrefix.empty())
        _context.socketPrefix = std::to_string(static_cast<int>(getpid()));
    _link = std::make_unique<Link>(this, _name, _context.channelType);

    if (_context.log2file)
        addTask([=] { setAttribute("logToFile", {_context.log2file}); });

    if (_context.defaultConfigurationFile)
        Log::get() << Log::MESSAGE << "No filename specified, loading default file" << Log::endl;
    else
        Log::get() << Log::MESSAGE << "Loading file " << _context.configurationFile << Log::endl;

    Json::Value config;
    _status &= loadConfig(_context.configurationFile, config);

    if (_status)
        _config = config;
    else if (!_context.unitTest)
        return false;

    return true;
}

/*************/
void World::run()
{
    tracy::SetThreadName("World");

    if (!applyContext())
        return;

    assert(_link);
    assert(_link->isReady());

    if (!applyConfig())
        return;

    while (true)
    {
        FrameMarkStart("World");

        Timer::get() << "loop_world";
        Timer::get() << "loop_world_inner";
        std::lock_guard<std::mutex> lockConfiguration(_configurationMutex);

        {
            // Process tree updates
            ZoneScopedN("Process tree");
            Timer::get() << "tree_process";
            _tree.processQueue(true);
            Timer::get() >> "tree_process";

            // Execute waiting tasks
            executeTreeCommands();
            runTasks();
        }

        {
            ZoneScopedN("Send buffers");
            std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

            // Read and serialize new buffers
            Timer::get() << "serialize";
            std::vector<SerializedObject> serializedObjects;

            {
                ZoneScopedN("Serialize buffers");
                for (auto& [name, object] : _objects)
                {
                    ZoneScopedN("Serialize one buffer");
                    ZoneName(name.c_str(), name.size());

                    object->runTasks();
                    object->update();
                    if (auto bufferObject = std::dynamic_pointer_cast<BufferObject>(object); bufferObject)
                    {
                        if (bufferObject->wasUpdated())
                        {
                            auto serializedObject = bufferObject->serialize();
                            bufferObject->setNotUpdated();
                            serializedObjects.push_back(std::move(serializedObject));
                        }
                    }
                }
                Timer::get() >> "serialize";
            }

            // Wait for previous buffers to be uploaded
            {
                ZoneScopedN("Wait for buffers to be sent");
                _link->waitForBufferSending(std::chrono::milliseconds(50)); // Maximum time to wait for frames to arrive
                sendMessage(Constants::ALL_PEERS, "uploadTextures", {});
                Timer::get() >> "upload";
            }

            // Ask for the upload of the new buffers, during the next world loop
            {
                ZoneScopedN("Prepare sending next buffers");
                Timer::get() << "upload";
                for (auto& serializedObject : serializedObjects)
                    _link->sendBuffer(std::move(serializedObject));
            }
        }

        if (_quit)
        {
            for (auto& s : _scenes)
                sendMessage(s.first, "quit", {});
            break;
        }

        {
            ZoneScopedN("Propagate tree");
            Timer::get() << "tree_propagate";
            updateTreeFromObjects();
            propagateTree();
            Timer::get() >> "tree_propagate";
        }

        // Sync with buffer object update
        Timer::get() >> "loop_world_inner";
        auto elapsed = Timer::get().getDuration("loop_world_inner");
        waitSignalBufferObjectUpdated(std::max<uint64_t>(1, 1e6 / (float)_worldFramerate - elapsed));

        // Sync to world framerate
        Timer::get() >> "loop_world";

        FrameMarkEnd("World")
    }
}

/*************/
void World::addToWorld(const std::string& type, const std::string& name)
{
    // BufferObject derived types have a counterpart on this side
    if (!_factory->isSubtype<BufferObject>(type))
        return;

    auto object = _factory->create(type);
    auto realName = name;
    if (object)
    {
        object->setName(name);
        _objects[name] = object;
    }
}

/*************/
bool World::applyConfig()
{
    std::lock_guard<std::mutex> lockConfiguration(_configurationMutex);

    // We first destroy all scene and objects
    _scenes.clear();
    _objects.clear();
    _masterSceneName = "";

    try
    {
        // Get the list of all scenes, and create them
        if (!_config.isMember("scenes"))
        {
            Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Error while getting scenes configuration" << Log::endl;
            return false;
        }

        const Json::Value& scenes = _config["scenes"];
        for (const auto& sceneName : scenes.getMemberNames())
        {
            std::string sceneAddress = scenes[sceneName].isMember("address") ? scenes[sceneName]["address"].asString() : "localhost";
            std::string sceneDisplay = scenes[sceneName].isMember("display") ? scenes[sceneName]["display"].asString() : "";
            bool spawn = scenes[sceneName].isMember("spawn") ? scenes[sceneName]["spawn"].asBool() : true;

            if (!addScene(sceneName, sceneDisplay, sceneAddress, spawn && _context.spawnSubprocesses))
                continue;

            // Set the remaining parameters
            for (const auto& paramName : scenes[sceneName].getMemberNames())
            {
                auto values = Utils::jsonToValues(scenes[sceneName][paramName]);
                sendMessage(sceneName, paramName, values);
            }
        }

        // Reseeds the world branch into the Scene's trees
        propagatePath("/world");

        // Configure each scenes
        // The first scene is the master one, and also receives some ghost objects
        // First, set the master scene
        sendMessage(_masterSceneName, "setMaster", {_configFilename});

        // Then, we create the objects
        for (const auto& scene : _scenes)
        {
            const Json::Value& objects = _config["scenes"][scene.first]["objects"];
            if (!objects)
                continue;

            // Create the objects
            auto sceneMembers = objects.getMemberNames();
            for (const auto& objectName : objects.getMemberNames())
            {
                if (!objects[objectName].isMember("type"))
                    continue;

                setAttribute("addObject", {objects[objectName]["type"].asString(), objectName, scene.first, false});
            }

            sendMessage(Constants::ALL_PEERS, "runInBackground", {_context.hide});
        }

        // Make sure all objects have been created in every Scene, by sending a sync
        // message
        for (const auto& s : _scenes)
            sendMessageWithAnswer(s.first, "sync");

        // Then we link the objects together
        for (auto& s : _scenes)
        {
            const Json::Value& scene = _config["scenes"][s.first];
            auto sceneMembers = scene.getMemberNames();
            const Json::Value& links = scene["links"];
            if (!links)
                continue;

            for (auto& link : links)
            {
                if (link.size() < 2)
                    continue;
                addTask([=]() { sendMessage(Constants::ALL_PEERS, "link", {link[0].asString(), link[1].asString()}); });
            }
        }

        // Configure the objects
        for (auto& s : _scenes)
        {
            const Json::Value& objects = _config["scenes"][s.first]["objects"];
            if (!objects)
                continue;

            // Create the objects
            for (const auto& objectName : objects.getMemberNames())
            {
                auto& obj = objects[objectName];

                addTask([=]() {
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

                        auto values = Utils::jsonToValues(attr);
                        values.push_front(objMembers[idxAttr]);
                        values.push_front(objectName);
                        setAttribute("sendAll", values);

                        idxAttr++;
                    }
                });
            }
        }

        // Lastly, configure this very World
        // This happens last as some parameters are sent to Scenes (like blending
        // computation)
        if (_config.isMember("world"))
        {
            const Json::Value jsWorld = _config["world"];
            auto worldMember = jsWorld.getMemberNames();
            int idx{0};
            for (const auto& attr : jsWorld)
            {
                auto values = Utils::jsonToValues(attr);
                std::string paramName = worldMember[idx];
                setAttribute(paramName, values);
                idx++;
            }
        }
    }
    catch (...)
    {
        Log::get() << Log::ERROR << "Exception caught while applying configuration from file " << _configFilename << Log::endl;
        return false;
    }

// Also, enable the master clock if it was not enabled
#if HAVE_PORTAUDIO
    addTask([&]() {
        if (!_clock)
            _clock = std::make_unique<LtcClock>(true);
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

    return true;
}

/*************/
bool World::addScene(const std::string& sceneName, const std::string& sceneDisplay, const std::string& sceneAddress, bool spawn)
{
    if (sceneAddress == "localhost")
    {
        std::string display{""};
        std::string worldDisplay{""};
#if HAVE_LINUX
        auto regDisplayFull = std::regex("(:[0-9]\\.[0-9])", std::regex_constants::extended);
        auto regDisplayInt = std::regex("[0-9]", std::regex_constants::extended);
        std::smatch match;

        if (getenv("DISPLAY") != nullptr)
        {
            worldDisplay = getenv("DISPLAY");
            if (!worldDisplay.empty() && worldDisplay.find(".") == std::string::npos)
                worldDisplay += ".0";
        }
                                                                                                                                       
        display = "DISPLAY=" + worldDisplay;
        if (!sceneDisplay.empty())
        {
            if (std::regex_match(sceneDisplay, match, regDisplayFull))
                display = "DISPLAY=" + sceneDisplay;
            else if (std::regex_match(sceneDisplay, match, regDisplayInt))
                display = "DISPLAY=:" + _context.displayServer + "." + sceneDisplay;
        }

        if (_context.forcedDisplay)
        {
            if (std::regex_match(*_context.forcedDisplay, match, regDisplayFull))
                display = "DISPLAY=" + *_context.forcedDisplay;
            else if (std::regex_match(*_context.forcedDisplay, match, regDisplayInt))
                display = "DISPLAY=:" + _context.displayServer + "." + *_context.forcedDisplay;
        }
#endif

        int pid = -1;
        if (spawn)
        {
            _sceneLaunched = false;

            // Spawn a new process containing this Scene
            Log::get() << Log::MESSAGE << "World::" << __FUNCTION__ << " - Starting a Scene in another process" << Log::endl;

            std::string cmd = _context.executablePath;
            std::string debug = (Log::get().getVerbosity() == Log::DEBUGGING) ? "-d" : "";
            std::string timer = Timer::get().isDebug() ? "-t" : "";
            std::string slave = "--child";
            std::string xauth = "XAUTHORITY=" + Utils::getHomePath() + "/.Xauthority";

            // Constructing arguments
            std::vector<char*> argv = {const_cast<char*>(cmd.c_str()), const_cast<char*>(slave.c_str())};
            if (!_context.socketPrefix.empty())
            {
                argv.push_back((char*)"--prefix");
                argv.push_back(const_cast<char*>(_context.socketPrefix.c_str()));
            }
            if (!debug.empty())
                argv.push_back(const_cast<char*>(debug.c_str()));
            if (!timer.empty())
                argv.push_back(const_cast<char*>(timer.c_str()));

            argv.push_back((char*)"--ipc");
            if (_context.channelType == Link::ChannelType::zmq)
                argv.push_back((char*)"zmq");
#if HAVE_SHMDATA
            else if (_context.channelType == Link::ChannelType::shmdata)
                argv.push_back((char*)"shmdata");
#endif

            argv.push_back(const_cast<char*>(sceneName.c_str()));
            argv.push_back(nullptr);

            // Constructing environment variables
            std::vector<char*> env = {const_cast<char*>(display.c_str()), const_cast<char*>(xauth.c_str()), nullptr};

            int status = posix_spawn(&pid, cmd.c_str(), nullptr, nullptr, argv.data(), env.data());
            if (status != 0)
                Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Error while spawning process for scene " << sceneName << Log::endl;

            _link->connectTo(sceneName);

            // We wait for the child process to be launched
            std::unique_lock<std::mutex> lockChildProcess(_childProcessMutex);
            while (!_sceneLaunched)
            {
                if (std::cv_status::timeout == _childProcessConditionVariable.wait_for(lockChildProcess, std::chrono::seconds(Constants::CONNECTION_TIMEOUT)))
                {
                    Log::get() << Log::ERROR << "World::" << __FUNCTION__ << " - Timeout when trying to connect to newly spawned scene \"" << sceneName << "\". Exiting."
                               << Log::endl;
                    _quit = true;
                    return false;
                }
            }
        }

        _scenes[sceneName] = pid;
        if (_masterSceneName.empty())
            _masterSceneName = sceneName;

        // Initialize the communication
        _link->connectTo(sceneName);

        return true;
    }
    else
    {
        Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Non-local scenes are not implemented yet" << Log::endl;
        return false;
    }
}

/*************/
std::string World::getObjectsAttributesDescriptions()
{
    Json::Value root;

    auto formatDescription = [](const std::string desc, const Values& argTypes) -> std::string {
        std::string descriptionStr = "[";
        for (uint32_t i = 0; i < argTypes.size(); ++i)
        {
            descriptionStr += argTypes[i].as<std::string>();
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

        root[obj->getType() + "_short_description"] = localFactory.getShortDescription(type);
        root[obj->getType() + "_description"] = localFactory.getDescription(type);

        auto attributesDescriptions = obj->getAttributesDescriptions();
        int addedAttribute = 0;
        root[obj->getType()] = Json::Value();
        for (auto& d : attributesDescriptions)
        {
            // We only keep attributes with a valid documentation
            // The other ones are inner attributes
            if (d[1].as<std::string>().size() == 0)
                continue;

            // We also don't keep attributes with no argument types
            if (d[2].as<Values>().size() == 0)
                continue;

            root[obj->getType()][d[0].as<std::string>()] = formatDescription(d[1].as<std::string>(), d[2].as<Values>());

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

        root["world"][d[0].as<std::string>()] = formatDescription(d[1].as<std::string>(), d[2].as<Values>());
    }

    setlocale(LC_NUMERIC,
        "C"); // Needed to make sure numbers are written with commas
    std::string jsonString;
    jsonString = root.toStyledString();

    return jsonString;
}

/*************/
void World::saveConfig()
{
    setlocale(LC_NUMERIC,
        "C"); // Needed to make sure numbers are written with commas

    // Local objects configuration can differ from the scenes objects,
    // as their type is not necessarily identical
    for (const auto& sceneName : _config["scenes"].getMemberNames())
    {
        if (!_tree.hasBranchAt("/" + sceneName))
            continue;

        // Set the scene configuration from what was received in the previous loop
        auto scene = getRootConfigurationAsJson(sceneName);
        for (const auto& attr : scene.getMemberNames())
        {
            if (attr != "objects")
            {
                _config["scenes"][sceneName][attr] = scene[attr];
            }
            else
            {
                Json::Value::Members objectNames = scene["objects"].getMemberNames();

                _config["scenes"][sceneName][attr] = Json::Value();
                Json::Value& objects = _config["scenes"][sceneName]["objects"];

                for (auto& m : objectNames)
                    for (const auto& a : scene["objects"][m].getMemberNames())
                        objects[m][a] = scene["objects"][m][a];
            }
        }
    }

    // Configuration from the world
    _config["description"] = Constants::FILE_CONFIGURATION;
    _config["version"] = std::string(PACKAGE_VERSION);
    auto worldConfiguration = getRootConfigurationAsJson("world");
    for (const auto& attr : worldConfiguration.getMemberNames())
    {
        _config["world"][attr] = worldConfiguration[attr];
    }

    setlocale(LC_NUMERIC,
        "C"); // Needed to make sure numbers are written with commas
    std::ofstream out(_configFilename, std::ios::binary);
    out << _config.toStyledString();
    out.close();
}

/*************/
void World::saveProject(const std::string& filename)
{
    try
    {
        setlocale(LC_NUMERIC,
            "C"); // Needed to make sure numbers are written with commas

        auto root = Json::Value(); // Haha, auto root...
        root["description"] = Constants::FILE_PROJECT;
        root["version"] = std::string(PACKAGE_VERSION);
        root["links"] = Json::Value();

        // Here, we don't care about which Scene holds which object, as objects with
        // the same name in different Scenes are necessarily clones
        std::set<std::pair<std::string, std::string>> existingLinks{}; // We keep a list of already existing links
        for (auto& s : _scenes)
        {
            auto config = getRootConfigurationAsJson(s.first);

            for (auto& v : config["links"])
            {
                // Only keep links to partially saved types
                bool isSavableType = _factory->isProjectSavable(config["objects"][v[0].asString()]["type"].asString());
                // If the object is linked to a camera, we save the link as
                // "saved to all available cameras"
                bool isLinkedToCam = (config["objects"][v[1].asString()]["type"].asString() == "camera");

                if (isLinkedToCam)
                    v[1] = Constants::CAMERA_LINK;

                auto link = std::make_pair<std::string, std::string>(v[0].asString(), v[1].asString());
                if (existingLinks.find(link) == existingLinks.end())
                    existingLinks.insert(link);
                else
                    continue;

                if (isSavableType)
                    root["links"].append(v);
            }

            for (const auto& member : config["objects"].getMemberNames())
            {
                // We only save configuration for non Scene-specific objects, which are
                // one of the following:
                if (!_factory->isProjectSavable(config["objects"][member]["type"].asString()))
                    continue;

                for (const auto& attr : config["objects"][member].getMemberNames())
                    root["objects"][member][attr] = config["objects"][member][attr];
            }
        }

        std::ofstream out(filename, std::ios::binary);
        out << root.toStyledString();
        out.close();
    }
    catch (...)
    {
        Log::get() << Log::ERROR << "Exception caught while saving file " << filename << Log::endl;
    }
}

/*************/
std::vector<std::string> World::getObjectsOfType(const std::string& type) const
{
    std::vector<std::string> objectList;

    for (const auto& rootName : _tree.getBranchList())
    {
        auto objectsPath = "/" + rootName + "/objects";
        for (const auto& objectName : _tree.getBranchListAt(objectsPath))
        {
            if (type.empty())
                objectList.push_back(objectName);

            auto typePath = objectsPath + "/" + objectName + "/type";
            assert(_tree.hasLeafAt(typePath));
            Value typeValue;
            _tree.getValueForLeafAt(typePath, typeValue);
            if (typeValue[0].as<std::string>() == type)
                objectList.push_back(objectName);
        }
    }

    std::sort(objectList.begin(), objectList.end());
    objectList.erase(std::unique(objectList.begin(), objectList.end()), objectList.end());

    return objectList;
}

/*************/
bool World::handleSerializedObject(const std::string& name, SerializedObject& obj)
{
    if (!RootObject::handleSerializedObject(name, obj))
    {
        // At this point, the serialized object should already hold its target name
        // as the first serialized member. Hence we do not need to add it.
        _link->sendBuffer(std::move(obj));
    }
    return true;
}

/*************/
void World::leave(int /*signal_value*/)
{
    Log::get() << "World::" << __FUNCTION__ << " - Received a SIG event. Quitting." << Log::endl;
    _that->_quit = true;
}

/*************/
bool World::copyCameraParameters(const std::string& filename)
{
    // List of copyable types
    static std::vector<std::string> copyableTypes{"camera", "warp"};

    Json::Value config;
    if (!Utils::loadJsonFile(filename, config))
        return false;

    // Get the scene names from this other configuration file
    for (const auto& s : config["scenes"].getMemberNames())
    {
        // Look for the cameras in the configuration file
        for (const auto& name : config["scenes"][s]["objects"].getMemberNames())
        {
            Json::Value& obj = config["scenes"][s]["objects"][name];
            if (std::find(copyableTypes.begin(), copyableTypes.end(), obj["type"].asString()) == copyableTypes.end())
                continue;

            // Go through the camera attributes
            for (const auto& attrName : obj.getMemberNames())
            {
                Json::Value& attr = obj[attrName];
                if (attrName == "type")
                    continue;

                auto values = Utils::jsonToValues(attr);

                // Send the new values for this attribute
                _tree.setValueForLeafAt("/" + s + "/objects/" + name + "/attributes/" + attrName, values);
            }
        }
    }

    return true;
}

/*************/
bool World::loadConfig(const std::string& filename, Json::Value& configuration)
{
    if (!Utils::loadJsonFile(filename, configuration))
        return false;

    if (!Utils::checkAndUpgradeConfiguration(configuration))
        return false;

    _configFilename = filename;
    _configurationPath = Utils::getPathFromFilePath(_configFilename);
    _mediaPath = Utils::getHomePath(); // By default, home path
    return true;
}

/*************/
bool World::loadProject(const std::string& filename)
{
    try
    {
        Json::Value partialConfig;
        if (!Utils::loadJsonFile(filename, partialConfig))
            return false;

        if (!partialConfig.isMember("description") || partialConfig["description"].asString() != Constants::FILE_PROJECT)
            return false;

        _projectFilename = filename;
        // The configuration path is overriden with the project file path
        _configurationPath = Utils::getPathFromFilePath(filename);

        // Now, we apply the configuration depending on the current state
        // Meaning, we replace objects with the same name, create objects with
        // non-existing name, and delete objects which are not in the partial config

        // Delete existing objects
        for (const auto& s : _scenes)
        {
            const Json::Value& sceneObjects = _config["scenes"][s.first]["objects"];
            for (const auto& member : sceneObjects.getMemberNames())
            {
                if (!sceneObjects[member].isMember("type"))
                    continue;
                if (_factory->isProjectSavable(sceneObjects[member]["type"].asString()))
                    setAttribute("deleteObject", {member});
            }
        }

        // Create new objects
        for (const auto& objectName : partialConfig["objects"].getMemberNames())
        {
            if (!partialConfig["objects"][objectName].isMember("type"))
                continue;
            setAttribute("addObject", {partialConfig["objects"][objectName]["type"].asString(), objectName, "", false});
        }

        // Handle the links
        // We will need a list of all cameras
        for (const auto& link : partialConfig["links"])
        {
            if (link.size() != 2)
                continue;
            auto source = link[0].asString();
            auto sink = link[1].asString();

            addTask([=]() {
                if (sink != Constants::CAMERA_LINK)
                {
                    sendMessage(Constants::ALL_PEERS, "link", {link[0].asString(), link[1].asString()});
                }
                else
                {
                    auto cameraNames = getObjectsOfType("camera");
                    for (const auto& camera : cameraNames)
                        sendMessage(Constants::ALL_PEERS, "link", {link[0].asString(), camera});
                }
            });
        }

        // Configure the objects
        for (const auto& objectName : partialConfig["objects"].getMemberNames())
        {
            auto& obj = partialConfig["objects"][objectName];
            auto configPath = Utils::getPathFromFilePath(_configFilename);

            addTask([=]() {
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

                    auto values = Utils::jsonToValues(attr);
                    values.push_front(objMembers[idxAttr]);
                    values.push_front(objectName);
                    setAttribute("sendAll", values);

                    idxAttr++;
                }
            });
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
void World::registerAttributes()
{
    addAttribute("addObject",
        [&](const Values& args) {
            addTask([=]() {
                auto type = args[0].as<std::string>();
                auto name = args.size() < 2 ? "" : args[1].as<std::string>();
                auto scene = args.size() < 3 ? "" : args[2].as<std::string>();
                auto checkName = args.size() < 4 ? true : args[3].as<bool>();

                std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);

                if (checkName && (name.empty() || !_nameRegistry.registerName(name)))
                    name = _nameRegistry.generateName(type);

                if (scene.empty())
                {
                    addToWorld(type, name);
                    for (auto& s : _scenes)
                    {
                        sendMessage(s.first, "addObject", {type, name, s.first});
                        sendMessageWithAnswer(s.first, "sync");
                    }
                }
                else
                {
                    addToWorld(type, name);
                    sendMessage(scene, "addObject", {type, name, scene});
                    if (scene != _masterSceneName)
                        sendMessage(_masterSceneName, "addObject", {type, name, scene});
                    sendMessageWithAnswer(scene, "sync");
                }

                set(name, "configFilePath", {Utils::getPathFromFilePath(_configFilename)}, false);
            });

            return true;
        },
        {'s'});
    setAttributeDescription("addObject", "Add an object to the scenes");

    addAttribute("sceneLaunched",
        [&](const Values&) {
            std::lock_guard<std::mutex> lockChildProcess(_childProcessMutex);
            _sceneLaunched = true;
            _childProcessConditionVariable.notify_all();
            return true;
        },
        {});
    setAttributeDescription("sceneLaunched", "Message sent by Scenes to confirm they are running");

    addAttribute("deleteObject",
        [&](const Values& args) {
            addTask([=]() {
                std::lock_guard<std::recursive_mutex> lockObjects(_objectsMutex);
                auto objectName = args[0].as<std::string>();

                // Delete the object here
                _nameRegistry.unregisterName(objectName);
                auto objectIt = _objects.find(objectName);
                if (objectIt != _objects.end())
                    _objects.erase(objectIt);

                // Ask for Scenes to delete the object
                sendMessage(Constants::ALL_PEERS, "deleteObject", args);

                for (const auto& s : _scenes)
                    sendMessageWithAnswer(s.first, "sync");
            });

            return true;
        },
        {'s'});
    setAttributeDescription("deleteObject", "Delete an object given its name");

    addAttribute("link",
        [&](const Values& args) {
            addTask([=]() { sendMessage(Constants::ALL_PEERS, "link", args); });
            return true;
        },
        {'s', 's'});
    setAttributeDescription("link", "Link the two given objects");

    addAttribute("unlink",
        [&](const Values& args) {
            addTask([=]() { sendMessage(Constants::ALL_PEERS, "unlink", args); });
            return true;
        },
        {'s', 's'});
    setAttributeDescription("unlink", "Unlink the two given objects");

    addAttribute("loadConfig",
        [&](const Values& args) {
            auto filename = args[0].as<std::string>();
            runAsyncTask([=]() {
                Json::Value config;
                if (loadConfig(filename, config))
                {
                    for (auto& [sceneName, scenePid] : _scenes)
                    {
                        sendMessage(sceneName, "quit");
                        _link->disconnectFrom(sceneName);
                        if (scenePid != -1)
                            waitpid(scenePid, nullptr, 0);
                    }

                    _masterSceneName = "";

                    _config = config;
                    applyConfig();
                }
            });
            return true;
        },
        {'s'});
    setAttributeDescription("loadConfig", "Load the given configuration file");

    addAttribute("copyCameraParameters",
        [&](const Values& args) {
            auto filename = args[0].as<std::string>();
            addTask([=]() { copyCameraParameters(filename); });
            return true;
        },
        {'s'});
    setAttributeDescription("copyCameraParameters",
        "Copy the camera parameters from the given "
        "configuration file (based on camera names)");

    addAttribute("pong",
        [&](const Values& args) {
            Timer::get() >> ("pingScene " + args[0].as<std::string>());
            return true;
        },
        {'s'});

    addAttribute("quit",
        [&](const Values&) {
            _quit = true;
            return true;
        },
        {});
    setAttributeDescription("quit", "Ask the world to quit");

    addAttribute("replaceObject",
        [&](const Values& args) {
            auto objName = args[0].as<std::string>();
            auto objType = args[1].as<std::string>();
            auto objAlias = args[2].as<std::string>();
            std::vector<std::string> targets;
            for (uint32_t i = 3; i < args.size(); ++i)
                targets.push_back(args[i].as<std::string>());

            if (!_factory->isCreatable(objType))
                return false;

            setAttribute("deleteObject", {objName});
            setAttribute("addObject", {objType, objName, "", false});
            addTask([=]() {
                for (const auto& t : targets)
                    setAttribute("sendAllScenes", {"link", objName, t});
            });
            return true;
        },
        {'s', 's'});
    setAttributeDescription("replaceObject",
        "Replace the given object by an object of the given type, with the given alias, and links the new object to the objects given by the following parameters");

    addAttribute("save",
        [&](const Values& args) {
            if (args.size() != 0)
                _configFilename = args[0].as<std::string>();

            addTask([=]() {
                Log::get() << "Saving configuration to " << _configFilename << Log::endl;
                saveConfig();
            });
            return true;
        },
        {});
    setAttributeDescription("save", "Save the configuration to the current file (or a new one if a name is given as parameter)");

    addAttribute("saveProject",
        [&](const Values& args) {
            auto filename = args[0].as<std::string>();
            addTask([this, filename]() {
                Log::get() << "Saving project to " << filename << Log::endl;
                saveProject(filename);
            });
            return true;
        },
        {'s'});
    setAttributeDescription("saveProject", "Save only the configuration of images, textures and meshes");

    addAttribute("loadProject",
        [&](const Values& args) {
            auto filename = args[0].as<std::string>();
            addTask([this, filename]() {
                Log::get() << "Loading partial configuration from " << filename << Log::endl;
                loadProject(filename);
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
        {'b'});
    setAttributeDescription("logToFile", "If true, the process holding the World will try to write log to file");

    addAttribute("sendAll",
        [&](const Values& args) {
            addTask([=]() {
                auto name = args[0].as<std::string>();
                auto attr = args[1].as<std::string>();
                auto values = args;

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
            auto attr = args[0].as<std::string>();
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
                auto attr = args[0].as<std::string>();
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
            if (args[0].as<bool>())
            {
                addPeriodicTask("pingTest", [&]() {
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
                removePeriodicTask("pingTest");
            }

            return true;
        },
        {'b'});
    setAttributeDescription("pingTest", "Activate ping test if true");

    addAttribute("swapTest",
        [&](const Values& args) {
            _swapSynchronizationTesting = args[0].as<int>();
            if (_swapSynchronizationTesting)
            {
                addPeriodicTask("swapTest", [&]() {
                    sendMessage(Constants::ALL_PEERS, "swapTest", {1});
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
                        sendMessage(Constants::ALL_PEERS, "swapTestColor", {color[0], color[1], color[2], color[3]});

                    frameNbr = (frameNbr + 1) % _swapSynchronizationTesting;
                });
            }
            else
            {
                removePeriodicTask("swapTest");
                addTask([&]() { sendMessage(Constants::ALL_PEERS, "swapTest", {0}); });
            }
            return true;
        },
        {'i'});
    setAttributeDescription("swapTest", "Activate video swap test if set to anything but 0");

    addAttribute("wireframe",
        [&](const Values& args) {
            addTask([=]() { sendMessage(Constants::ALL_PEERS, "wireframe", args); });

            return true;
        },
        {'b'});
    setAttributeDescription("wireframe", "Show all meshes as wireframes if true");

#if HAVE_LINUX
    addAttribute("forceRealtime",
        [&](const Values& args) {
            _enforceRealtime = args[0].as<bool>();

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
        [&]() -> Values { return {_enforceRealtime}; },
        {'b'});
    setAttributeDescription("forceRealtime", "Ask the scheduler to run Splash with realtime priority.");
#endif

    addAttribute("framerate",
        [&](const Values& args) {
            _worldFramerate = std::max(1, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {(int)_worldFramerate}; },
        {'i'});
    setAttributeDescription("framerate", "Set the minimum refresh rate for the world (adapted to video framerate)");

#if HAVE_PORTAUDIO
    addAttribute("clockDeviceName",
        [&](const Values& args) {
            addTask([=]() {
                auto clockDeviceName = args[0].as<std::string>();
                if (clockDeviceName != _clockDeviceName)
                {
                    _clockDeviceName = clockDeviceName;
                    _clock = std::make_unique<LtcClock>(true, _clockDeviceName);
                }
            });

            return true;
        },
        [&]() -> Values { return {_clockDeviceName}; },
        {'s'});
    setAttributeDescription("clockDeviceName", "Set the audio device name from which to read the LTC clock signal");
#endif

    addAttribute("configurationPath", [&](const Values& /*args*/) { return true; }, [&]() -> Values { return {_configurationPath}; }, {'s'});
    setAttributeDescription("configurationPath", "Path to the configuration files");

    addAttribute("mediaPath",
        [&](const Values& args) {
            auto path = args[0].as<std::string>();
            if (Utils::isDir(path))
                _mediaPath = args[0].as<std::string>();
            return true;
        },
        [&]() -> Values { return {_mediaPath}; },
        {'s'});
    setAttributeDescription("mediaPath", "Path to the media files");

    addAttribute("looseClock",
        [&](const Values& args) {
            Timer::get().setLoose(args[0].as<bool>());
            return true;
        },
        [&]() -> Values { return {Timer::get().isLoose()}; },
        {'b'});
    setAttributeDescription("looseClock", "Master clock is not a hard constraints if true");

    addAttribute("clock", [&](const Values& /*args*/) { return true; }, [&]() -> Values { return {Timer::getTime()}; }, {});
    setAttributeDescription("clock", "Current World clock (not settable)");

    addAttribute("masterClock",
        [&](const Values& /*args*/) { return true; },
        [&]() -> Values {
            Timer::Point masterClock;
            if (Timer::get().getMasterClock(masterClock))
                return {masterClock.years, masterClock.months, masterClock.days, masterClock.hours, masterClock.mins, masterClock.secs, masterClock.frame, masterClock.paused};
            else
                return {};
        },
        {});
    setAttributeDescription("masterClock", "Current World master clock (not settable)");

    RootObject::registerAttributes();
}

/*************/
void World::initializeTree()
{
    _tree.setName(_name);
}

} // namespace Splash
