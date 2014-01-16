#include "world.h"
#include "timer.h"

#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;
using namespace std;

namespace Splash {

/*************/
mutex World::_callbackMutex;
deque<vector<int>> World::_keys;
deque<vector<int>> World::_mouseBtn;
vector<double> World::_mousePos;

/*************/
World::World(int argc, char** argv)
{
    parseArguments(argc, argv);

    init();

    _eye = vec3(2.0, 2.0, 1.0);
    _target = vec3(0.0, 0.0, 0.3);
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

        // Render the local World windows
        render();

        // Then render the scenes
        for (auto& s : _scenes)
            run &= !s.second->render();

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
    // We grab also Objects for showing them locally
    if (type != "image" && type != "mesh" && type != "image_shmdata" && type != "object")
        return;

    glfwMakeContextCurrent(_window->get());

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
        else if (type == string("object"))
        {
            ObjectPtr obj(new Object());
            obj->setId(getId());
            object = dynamic_pointer_cast<BaseObject>(obj);
            _objects[name] = obj;
        }
    }

    if (_objectDest.find(name) == _objectDest.end())
    {
        _objectDest[name] = vector<string>();
        _objectDest[name].emplace_back(destination);
    }
    else
    {
        bool isPresent = false;
        for (auto d : _objectDest[name])
            if (d == destination)
                isPresent = true;
        if (!isPresent)
            _objectDest[name].push_back(destination);
    }

    glfwMakeContextCurrent(NULL);
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
                linkLocally(link[0].asString(), link[1].asString());
            }
            idx++;
        }
    }
}

/*************/
mat4x4 World::computeViewProjectionMatrix()
{
    mat4x4 viewMatrix = lookAt(_eye, _target, glm::vec3(0.0, 0.0, 1.0));
    mat4x4 projMatrix = perspectiveFov(_fov, _width, _height, _near, _far);
    mat4x4 viewProjectionMatrix = projMatrix * viewMatrix;

    return viewProjectionMatrix;
}

/*************/
void World::init()
{
    glfwSetErrorCallback(World::glfwErrorCallback);

    // GLFW stuff
    if (!glfwInit())
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
        //_isInitialized = false;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, SPLASH_GL_CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, SPLASH_GL_CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, SPLASH_GL_DEBUG);
    glfwWindowHint(GLFW_SAMPLES, SPLASH_SAMPLES);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_VISIBLE, true);

    GLFWwindow* window = glfwCreateWindow(512, 512, "Splash::World", NULL, NULL);

    if (!window)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        //_isInitialized = false;
        return;
    }

    _window.reset(new GlWindow(window, window));
    glfwMakeContextCurrent(_window->get());
    //_isInitialized = true;
    glfwMakeContextCurrent(NULL);

    // Initialize the callbacks
    if (!glfwSetKeyCallback(_window->get(), World::keyCallback))
        SLog::log << Log::DEBUG << "World::" << __FUNCTION__ << " - Error while setting up key callback" << Log::endl;
    if (!glfwSetMouseButtonCallback(_window->get(), World::mouseBtnCallback))
        SLog::log << Log::DEBUG << "World::" << __FUNCTION__ << " - Error while setting up mouse button callback" << Log::endl;
    if (!glfwSetCursorPosCallback(_window->get(), World::mousePosCallback))
        SLog::log << Log::DEBUG << "World::" << __FUNCTION__ << " - Error while setting up mouse position callback" << Log::endl;
}

/*************/
void World::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    std::vector<int> keys {key, scancode, action, mods};
    _keys.push_back(keys);
}

/*************/
void World::mouseBtnCallback(GLFWwindow* win, int button, int action, int mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    std::vector<int> btn {button, action, mods};
    _mouseBtn.push_back(btn);
}

/*************/
void World::mousePosCallback(GLFWwindow* win, double xpos, double ypos)
{
    lock_guard<mutex> lock(_callbackMutex);
    std::vector<double> pos {xpos, ypos};
    _mousePos = move(pos);
}

/*************/
void World::linkLocally(string first, string second)
{
    glfwMakeContextCurrent(_window->get());

    if (_objects.find(first) != _objects.end() && _objects.find(second) != _objects.end())
    {
        if (dynamic_pointer_cast<Image>(_objects[first]).get() != nullptr && dynamic_pointer_cast<Object>(_objects[second]).get() != nullptr)
        {
            TexturePtr tex(new Texture());
            tex->setId(getId());
            ImagePtr img = dynamic_pointer_cast<Image>(_objects[first]);
            *tex = img;
            _textures.push_back(tex);
            dynamic_pointer_cast<Object>(_objects[second])->addTexture(tex);
        }
        else if (dynamic_pointer_cast<Mesh>(_objects[first]).get() != nullptr && dynamic_pointer_cast<Object>(_objects[second]).get() != nullptr)
        {
            GeometryPtr geom(new Geometry());
            geom->setId(getId());
            MeshPtr mesh = dynamic_pointer_cast<Mesh>(_objects[first]);
            geom->setMesh(mesh);
            dynamic_pointer_cast<Object>(_objects[second])->addGeometry(geom);
        }
    }

    glfwMakeContextCurrent(NULL);
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
void World::render()
{
    glfwMakeContextCurrent(_window->get());

    for (auto& t : _textures)
        t->update();

    int w, h;
    glfwGetWindowSize(_window->get(), &w, &h);
    glViewport(0, 0, w, h);
    _width = (float)w;
    _height = (float)h;

    glGetError();
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    for (auto& obj : _objects)
    {
        if (obj.second->getType() == "object")
        {
            ObjectPtr renderable = dynamic_pointer_cast<Object>(obj.second);
            renderable->activate();
            renderable->setViewProjectionMatrix(computeViewProjectionMatrix());
            renderable->draw();
            renderable->deactivate();
        }
    }

    glDisable(GL_DEPTH_TEST);

    glfwSwapBuffers(_window->get());

    GLenum error = glGetError();
    if (error)
        SLog::log << Log::WARNING << "World::" << __FUNCTION__ << " - Error while rendering the World window: " << error << Log::endl;

    glfwMakeContextCurrent(NULL);
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
