#include "./core/factory.h"

#include <json/json.h>

#include "./controller/controller_blender.h"
#include "./core/link.h"
#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/filter.h"
#include "./graphics/geometry.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/virtual_probe.h"
#include "./graphics/warp.h"
#include "./graphics/window.h"
#include "./image/image.h"
#include "./image/image_ffmpeg.h"
#include "./image/queue.h"
#include "./mesh/mesh.h"
#include "./sink/sink.h"
#include "./utils/log.h"
#include "./utils/timer.h"

#if HAVE_LINUX
#include "./image/image_v4l2.h"
#endif

#if HAVE_GPHOTO and HAVE_OPENCV
#include "./image/image_gphoto.h"
#endif

#if HAVE_SHMDATA
#include "./image/image_shmdata.h"
#include "./mesh/mesh_shmdata.h"
#include "./sink/sink_shmdata.h"
#include "./sink/sink_shmdata_encoded.h"
#endif

#if HAVE_PYTHON
#include "./controller/controller_pythonembedded.h"
#endif

#if HAVE_OSX
#include "./graphics/texture_syphon.h"
#endif

#if HAVE_OPENCV
#include "./image/image_opencv.h"
#endif

using namespace std;

namespace Splash
{

/*************/
Factory::Factory()
    : _root(nullptr)
{
    registerObjects();
}

/*************/
Factory::Factory(RootObject* root)
    : _root(root)
{
    _scene = dynamic_cast<Scene*>(_root);

    loadDefaults();
    registerObjects();
}

/*************/
void Factory::loadDefaults()
{
    auto defaultEnv = getenv(SPLASH_DEFAULTS_FILE_ENV);
    if (!defaultEnv)
        return;

    auto filename = string(defaultEnv);
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
        Log::get() << Log::WARNING << "Factory::" << __FUNCTION__ << " - Unable to open file " << filename << Log::endl;
        return;
    }

    Json::Value config;
    Json::Reader reader;

    bool success = reader.parse(contents, config);
    if (!success)
    {
        Log::get() << Log::WARNING << "Factory::" << __FUNCTION__ << " - Unable to parse file " << filename << Log::endl;
        Log::get() << Log::WARNING << reader.getFormattedErrorMessages() << Log::endl;
        return;
    }

    auto objNames = config.getMemberNames();
    for (auto& name : objNames)
    {
        unordered_map<string, Values> defaults;
        auto attrNames = config[name].getMemberNames();
        for (const auto& attrName : attrNames)
        {
            auto value = jsonToValues(config[name][attrName]);
            defaults[attrName] = value;
        }
        _defaults[name] = defaults;
    }
}

/*************/
Values Factory::jsonToValues(const Json::Value& values)
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
shared_ptr<GraphObject> Factory::create(const string& type)
{
    // Not all object types are listed here, only those which are available to the user are
    auto page = _objectBook.find(type);
    if (page != _objectBook.end())
    {
        auto object = page->second.builder(_root);

        auto defaultIt = _defaults.find(type);
        if (defaultIt != _defaults.end())
            for (auto& attr : defaultIt->second)
                object->setAttribute(attr.first, attr.second);

        if (object)
            object->setCategory(page->second.objectCategory);
        return object;
    }
    else
    {
        Log::get() << Log::WARNING << "Factory::" << __FUNCTION__ << " - Object type " << type << " does not exist" << Log::endl;
        return shared_ptr<GraphObject>(nullptr);
    }
}

/*************/
vector<string> Factory::getObjectTypes()
{
    vector<string> types;
    for (auto& page : _objectBook)
        types.push_back(page.first);
    return types;
}

/*************/
vector<string> Factory::getObjectsOfCategory(GraphObject::Category c)
{
    vector<string> types;
    for (auto& page : _objectBook)
        if (page.second.objectCategory == c)
            types.push_back(page.first);

    return types;
}

/*************/
string Factory::getShortDescription(const string& type)
{
    if (!isCreatable(type))
        return "";

    return _objectBook[type].shortDescription;
}

/*************/
string Factory::getDescription(const string& type)
{
    if (!isCreatable(type))
        return "";

    return _objectBook[type].description;
}

/*************/
bool Factory::isCreatable(const string& type)
{
    auto it = _objectBook.find(type);
    if (it == _objectBook.end())
    {
        Log::get() << Log::WARNING << "Factory::" << __FUNCTION__ << " - Object type " << type << " does not exist" << Log::endl;
        return false;
    }

    return true;
}

/*************/
bool Factory::isProjectSavable(const string& type)
{
    auto it = _objectBook.find(type);
    if (it == _objectBook.end())
    {
        Log::get() << Log::DEBUGGING << "Factory::" << __FUNCTION__ << " - Object type " << type << " does not exist" << Log::endl;
        return false;
    }

    return it->second.projectSavable;
}

/*************/
void Factory::registerObjects()
{
    _objectBook["blender"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Blender>(root)); },
        GraphObject::Category::MISC,
        "blender",
        "Controls the blending of all the cameras.");

    _objectBook["camera"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Camera>(root)); },
        GraphObject::Category::MISC,
        "camera",
        "Virtual camera which corresponds to a given videoprojector.");

    _objectBook["filter"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Filter>(root)); },
        GraphObject::Category::MISC,
        "filter",
        "Filter applied to textures. The default filter allows for standard image manipulation, the user can set his own GLSL shader.",
        true);

    _objectBook["geometry"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Geometry>(root)); },
        GraphObject::Category::MISC,
        "Geometry",
        "Intermediary object holding vertices, UV and normal coordinates of a projection surface.");

    _objectBook["image"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Image>(root)); },
        GraphObject::Category::IMAGE,
        "image",
        "Static image read from a file.",
        true);

#if HAVE_LINUX
    _objectBook["image_v4l2"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image_V4L2>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "Video4Linux2 input device",
        "Image object reading frames from a Video4Linux2 compatible input.",
        true);
#endif

    _objectBook["image_ffmpeg"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image_FFmpeg>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video",
        "Image object reading frames from a video file.",
        true);

#if HAVE_GPHOTO and HAVE_OPENCV
    _objectBook["image_gphoto"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image_GPhoto>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "digital camera",
        "Image object reading from from a GPhoto2 compatible camera.",
        true);
#endif

#if HAVE_SHMDATA
    _objectBook["image_shmdata"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image_Shmdata>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video through shared memory",
        "Image object reading frames from a Shmdata shared memory.",
        true);
#endif

#if HAVE_OPENCV
    _objectBook["image_opencv"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image_OpenCV>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "camera through opencv",
        "Image object reading frames from a OpenCV compatible camera.",
        true);
#endif

    _objectBook["mesh"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Mesh>(root)); },
        GraphObject::Category::MESH,
        "mesh from obj file",
        "Mesh (vertices and UVs) describing a projection surface, read from a .obj file.",
        true);

#if HAVE_SHMDATA
    _objectBook["mesh_shmdata"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Mesh_Shmdata>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<Mesh>(root));
            return object;
        },
        GraphObject::Category::MESH,
        "mesh through shared memory",
        "Mesh object reading data from a Shmdata shared memory.",
        true);
#endif

    _objectBook["sink"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Sink>(root)); },
        GraphObject::Category::MISC,
        "sink a texture to a host buffer",
        "Get the texture content to a host buffer. Only used internally.");

#if HAVE_SHMDATA
    _objectBook["sink_shmdata"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Sink_Shmdata>(root)); },
        GraphObject::Category::MISC,
        "sink a texture to shmdata file",
        "Outputs connected texture to a Shmdata shared memory.");

    _objectBook["sink_shmdata_encoded"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Sink_Shmdata_Encoded>(root)); },
        GraphObject::Category::MISC,
        "sink a texture as an encoded video to shmdata file",
        "Outputs texture as a compressed frame to a Shmdata shared memory.");
#endif

    _objectBook["object"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Object>(root)); },
        GraphObject::Category::MISC,
        "object",
        "Utility class used for specify which image is mapped onto which mesh.",
        true);

#if HAVE_PYTHON
    _objectBook["python"] = Page(
        [&](RootObject* root) {
            if (!root || (_scene && !_scene->isMaster()))
                return shared_ptr<GraphObject>(nullptr);
            return dynamic_pointer_cast<GraphObject>(make_shared<PythonEmbedded>(root));
        },
        GraphObject::Category::MISC,
        "python",
        "Allows for controlling Splash through a Python script.");
#endif

    _objectBook["queue"] = Page(
        [&](RootObject* root) {
            shared_ptr<GraphObject> object;
            if (!_scene)
                object = dynamic_pointer_cast<GraphObject>(make_shared<Queue>(root));
            else
                object = dynamic_pointer_cast<GraphObject>(make_shared<QueueSurrogate>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video queue",
        "Allows for creating a timed playlist of image sources.",
        true);

    _objectBook["texture_image"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Texture_Image>(root)); },
        GraphObject::Category::TEXTURE,
        "texture image",
        "Texture object created from an Image object.",
        true);

#if HAVE_OSX
    _objectBook["texture_syphon"] = Page(
        [&](RootObject* root) {
            if (!_scene)
                return shared_ptr<GraphObject>(nullptr);
            else
                return dynamic_pointer_cast<GraphObject>(make_shared<Texture_Syphon>(root));
        },
        GraphObject::Category::TEXTURE,
        "texture image through Syphon",
        "Texture object synchronized through Syphon.",
        true);
#endif

    _objectBook["virtual_probe"] = Page(
        [&](RootObject* root) {
            if (!_scene)
                return dynamic_pointer_cast<GraphObject>(make_shared<VirtualProbe>(nullptr));
            else
                return dynamic_pointer_cast<GraphObject>(make_shared<VirtualProbe>(root));
        },
        GraphObject::Category::MISC,
        "virtual probe to simulate a virtual projection surface",
        "Virtual screen used to simulate a virtual projection surface.",
        true);

    _objectBook["warp"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Warp>(root)); },
        GraphObject::Category::MISC,
        "warp",
        "Warping object, allows for deforming the output of a Camera.");

    _objectBook["window"] = Page([&](RootObject* root) { return dynamic_pointer_cast<GraphObject>(make_shared<Window>(root)); },
        GraphObject::Category::MISC,
        "window",
        "Window object, set to be shown on one or multiple physical outputs.");
}

} // namespace Splash
