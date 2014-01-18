#include "world.h"
#include "timer.h"

#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

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
}

/*************/
void World::run()
{
    applyConfig();

    while (true)
    {
        STimer::timer << "worldLoop";
        static float framerate = 0.f;
        if (_showFramerate)
        {
            framerate *= 0.9f;
            framerate += 0.1f * 1e6f / (float)std::max(STimer::timer["worldLoop"], 1ull);
            SLog::log << Log::MESSAGE << "World::" << __FUNCTION__ << " - Framerate: " << framerate << Log::endl;
        }

        bool run {true};

        // Update the local objects
        for (auto& o : _objects)
            o.second->update();
        // Send them the their destinations
        for (auto& o : _objects)
        {
            SerializedObject obj;
            if (dynamic_pointer_cast<Image>(o.second).get() != nullptr)
                obj = dynamic_pointer_cast<Image>(o.second)->serialize();
            else if (dynamic_pointer_cast<Mesh>(o.second).get() != nullptr)
                obj = dynamic_pointer_cast<Mesh>(o.second)->serialize();

            for (auto& dest : _objectDest[o.first])
                if (_scenes.find(dest) != _scenes.end())
                    _scenes[dest]->setFromSerializedObject(o.first, obj);
        }

        // Then render the scenes
        for (auto& s : _scenes)
            _threadPool->enqueue([&]() {
                run &= !s.second->render();
            });
        _threadPool->waitAllThreads();

        if (!run)
            break;

        // Match the desired FPS
        STimer::timer >> 1e6 / 60 >> "worldLoop";
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
        {
            ImagePtr image(new Image());
            image->setId(getId());
            object = dynamic_pointer_cast<BaseObject>(image);
            _objects[name] = image;
        }
        else if (type == string("image_shmdata"))
        {
            Image_ShmdataPtr image(new Image_Shmdata());
            image->setId(getId());
            object = dynamic_pointer_cast<BaseObject>(image);
            _objects[name] = image;
        }
        else if (type == string("mesh"))
        {
            MeshPtr mesh(new Mesh());
            mesh->setId(getId());
            object = dynamic_pointer_cast<BaseObject>(mesh);
            _objects[name] = mesh;
        }
    }

    // If the object is not registered yet, we add it with the specified destination
    // as well as the WORLD_SCENE destination
    if (_objectDest.find(name) == _objectDest.end())
    {
        _objectDest[name] = vector<string>();
        _objectDest[name].emplace_back(destination);
        _objectDest[name].emplace_back(SPLASH_WORLD_SCENE);
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
            ScenePtr scene(new Scene);
            _scenes[name] = scene;
        }
        else
        {
            SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Non-local scenes are not implemented yet" << Log::endl;
        }
    }

    // We also create a default, local scene
    {
        ScenePtr scene(new Scene);
        _scenes[SPLASH_WORLD_SCENE] = scene;
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
            // Also, some are added to the local scene
            if (type != "camera" && type != "window")
                _scenes[SPLASH_WORLD_SCENE]->add(type, name);

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
                // As well as in the local scene
                _scenes[SPLASH_WORLD_SCENE]->setAttribute(name, objMembers[idxAttr], values);

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
                // Link also in the local scene, at least objects which are present
                _scenes[SPLASH_WORLD_SCENE]->link(link[0].asString(), link[1].asString());
            }
            idx++;
        }

        // Lastly, in the local scene, connect all Objects to the single camera present
        // Also, this will give this scene a special behavior
        _scenes[SPLASH_WORLD_SCENE]->setAsWorldScene();
    }
}

/*************/
void World::init()
{
    _threadPool.reset(new ThreadPool(SPLASH_MAX_THREAD));
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
        else if (string(argv[idx]) == "--fps")
        {
            _showFramerate = true;
            idx++;
        }
        else
            idx++;
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
