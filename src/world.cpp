#include "world.h"
#include "timer.h"

#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <json/reader.h>
#include <json/writer.h>

using namespace glm;
using namespace std;

namespace Splash {
/*************/
World::World(int argc, char** argv)
{
    parseArguments(argc, argv);

    init();
}

/*************/
World::~World()
{
    SLog::log << Log::DEBUG << "World::~World - Destructor" << Log::endl;
    SThread::pool.waitAllThreads();
}

/*************/
void World::run()
{
    applyConfig();

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
                SerializedObjectPtr obj(new SerializedObject);
                if (dynamic_pointer_cast<BufferObject>(o.second).get() != nullptr)
                    if (dynamic_pointer_cast<BufferObject>(o.second)->wasUpdated()) // if the buffer has been updated
                        *obj = dynamic_pointer_cast<BufferObject>(o.second)->serialize();
                    else
                        return; // if not, exit this thread

                dynamic_pointer_cast<BufferObject>(o.second)->setNotUpdated();
                for (auto& dest : _objectDest[o.first])
                    if (_scenes.find(dest) != _scenes.end())
                            _scenes[dest]->setFromSerializedObject(o.first, *obj);
            }));
        }
        STimer::timer >> "upload";

        // Render the scenes
        bool run {true};
        for (auto& s : _scenes)
            run &= !s.second->render();

        if (!run)
            break;

        SThread::pool.waitThreads(threadIds);

        // Grab the signals from the scenes
        for (auto& s : _scenes)
        {
            map<string, vector<Value>> messages = s.second->getMessages();
            parseMessages(messages);
        }

        // Get the current FPS
        STimer::timer >> "worldLoop";
    }
}

/*************/
void World::addLocally(string type, string name, string destination)
{
    // Images and Meshes have a counterpart on this side
    if (type != "image" && type != "mesh" && type != "image_shmdata")
        return;

    BaseObjectPtr object;
    if (_objects.find(name) == _objects.end())
    {
        if (type == string("image"))
            object = dynamic_pointer_cast<BaseObject>(ImagePtr(new Image()));
        else if (type == string("image_shmdata"))
            object = dynamic_pointer_cast<BaseObject>(Image_ShmdataPtr(new Image_Shmdata()));
        else if (type == string("mesh"))
            object = dynamic_pointer_cast<BaseObject>(MeshPtr(new Mesh()));
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

    vector<string> scenes;

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
            string name = jsScenes[i]["name"].asString();
            ScenePtr scene(new Scene(name));
            _scenes[name] = scene;
        }
        else
        {
            SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Non-local scenes are not implemented yet" << Log::endl;
        }
    }

    // Configure each scenes
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
            s.second->add(type, name);

            // Some objects are also created on this side, and linked with the distant one
            addLocally(type, name, s.first);

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

                vector<Value> values;
                if (attr.isArray())
                    for (auto& v : attr)
                    {
                        if (v.isInt())
                            values.emplace_back(v.asInt());
                        else if (v.isDouble())
                            values.emplace_back(v.asFloat());
                        else
                            values.emplace_back(v.asString());
                    }
                else if (attr.isInt())
                    values.emplace_back(attr.asInt());
                else if (attr.isDouble())
                    values.emplace_back(attr.asFloat());
                else if (attr.isString())
                    values.emplace_back(attr.asString());

                s.second->setAttribute(name, objMembers[idxAttr], values);
                // We also set the attribute locally, if the object exists
                setAttribute(name, objMembers[idxAttr], values);

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
                s.second->link(link[0].asString(), link[1].asString());
            }
            idx++;
        }

        // Lastly, in the local scene, connect all Objects to the single camera present
    }
}

/*************/
void World::saveConfig()
{
    Json::Value root;
    root["scenes"] = Json::Value();

    // Get the configuration from the different scenes
    for (auto& s : _scenes)
    {
        Json::Value scene;
        scene["name"] = s.first;
        scene["address"] = "localhost"; // Distant scenes are not yet supported
        root["scenes"].append(scene);

        Json::Value config = s.second->getConfigurationAsJson();
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
    
    ofstream out(_configFilename, ios::binary);
    out << _config.toStyledString();
    out.close();
}

/*************/
void World::init()
{
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
    int idx = 0;
    while (idx < argc)
    {
        if ((string(argv[idx]) == "-o" || string(argv[idx]) == "--open") && idx + 1 < argc)
        {
            string filename = string(argv[idx + 1]);
            _status &= loadConfig(filename);
            idx += 2;
        }
        else if (string(argv[idx]) == "-d")
        {
            SLog::log.setVerbosity(Log::DEBUG);
            idx++;
        }
        else if (string(argv[idx]) == "-s" || string(argv[idx]) == "--silent")
        {
            SLog::log.setVerbosity(Log::NONE);
            idx++;
        }
        else
            idx++;
    }
}

/*************/
void World::parseMessages(map<string, vector<Value>> messages)
{
    for (auto& m : messages)
    {
        if (m.first == string("save"))
        {
            SLog::log << "Configuration saved" << Log::endl;
            saveConfig();
        }
    }
}

/*************/
void World::setAttribute(string name, string attrib, std::vector<Value> args)
{
    if (_objects.find(name) != _objects.end())
        _objects[name]->setAttribute(attrib, args);
}

/*************/
void World::glfwErrorCallback(int code, const char* msg)
{
    SLog::log << Log::WARNING << "World::glfwErrorCallback - " << msg << Log::endl;
}

}
