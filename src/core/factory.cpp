#include "./core/factory.h"

#include "./controller/controller_blender.h"
#include "./core/constants.h"
#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/filter.h"
#include "./graphics/filter_black_level.h"
#include "./graphics/filter_color_curves.h"
#include "./graphics/filter_custom.h"
#include "./graphics/geometry.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/virtual_probe.h"
#include "./graphics/warp.h"
#include "./graphics/window.h"
#include "./image/image.h"
#include "./image/image_ffmpeg.h"
#include "./image/image_list.h"
#include "./image/queue.h"
#include "./mesh/mesh.h"
#include "./network/link.h"
#include "./sink/sink.h"
#include "./utils/jsonutils.h"
#include "./utils/log.h"
#include "./utils/timer.h"

#if HAVE_LINUX
#include "./image/image_v4l2.h"
#endif

#if HAVE_GPHOTO and HAVE_OPENCV
#include "./image/image_gphoto.h"
#endif

#if HAVE_SH4LT
#include "./image/image_sh4lt.h"
#include "./sink/sink_sh4lt.h"
#include "./sink/sink_sh4lt_encoded.h"
#endif

#if HAVE_SHMDATA
#include "./image/image_ndi.h"
#include "./image/image_shmdata.h"
#include "./mesh/mesh_shmdata.h"
#include "./sink/sink_shmdata.h"
#include "./sink/sink_shmdata_encoded.h"
#endif

#if HAVE_PYTHON
#include "./controller/controller_pythonembedded.h"
#endif

#if HAVE_OPENCV
#include "./image/image_opencv.h"
#endif

namespace Splash
{

/*************/
Factory::Factory()
    : _root(nullptr)
{
    loadDefaults();
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
    auto defaultEnv = getenv(Constants::DEFAULT_FILE_ENV);
    if (!defaultEnv)
        return;

    auto filename = std::string(defaultEnv);
    Json::Value config;
    if (!Utils::loadJsonFile(filename, config))
        return;

    auto objNames = config.getMemberNames();
    for (auto& name : objNames)
    {
        std::unordered_map<std::string, Values> defaults;
        auto attrNames = config[name].getMemberNames();
        for (const auto& attrName : attrNames)
        {
            auto value = Utils::jsonToValues(config[name][attrName]);
            defaults[attrName] = value;
        }
        _defaults[name] = defaults;
    }
}

/*************/
std::shared_ptr<GraphObject> Factory::create(const std::string& type)
{
    // Not all object types are listed here, only those available to the user
    auto page = _objectBook.find(type);
    if (page != _objectBook.end())
    {
        auto object = page->second.builder(_root);

        auto defaultIt = _defaults.find(type);
        if (defaultIt != _defaults.end())
            for (const auto& attr : defaultIt->second)
                object->setAttribute(attr.first, attr.second);

        if (object)
            object->setCategory(page->second.objectCategory);
        return object;
    }
    else
    {
        Log::get() << Log::WARNING << "Factory::" << __FUNCTION__ << " - Object type " << type << " does not exist" << Log::endl;
        return std::shared_ptr<GraphObject>(nullptr);
    }
}

/*************/
const std::vector<std::string> Factory::getObjectTypes() const
{
    std::vector<std::string> types;
    for (const auto& page : _objectBook)
        types.push_back(page.first);
    return types;
}

/*************/
std::vector<std::string> Factory::getObjectsOfCategory(GraphObject::Category c)
{
    std::vector<std::string> types;
    for (const auto& page : _objectBook)
        if (page.second.objectCategory == c)
            types.push_back(page.first);

    return types;
}

/*************/
std::string Factory::getShortDescription(const std::string& type)
{
    if (!isCreatable(type))
        return "";

    return _objectBook[type].shortDescription;
}

/*************/
std::string Factory::getDescription(const std::string& type)
{
    if (!isCreatable(type))
        return "";

    return _objectBook[type].description;
}

/*************/
bool Factory::isCreatable(const std::string& type)
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
bool Factory::isProjectSavable(const std::string& type)
{
    auto it = _objectBook.find(type);
    if (it == _objectBook.end())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Factory::" << __FUNCTION__ << " - Object type " << type << " does not exist" << Log::endl;
#endif
        return false;
    }

    return it->second.projectSavable;
}

/*************/
void Factory::registerObjects()
{
    _objectBook["blender"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Blender>(root)); },
        GraphObject::Category::MISC,
        "blender",
        "Controls the blending of all the cameras.");

    _objectBook["camera"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Camera>(root)); },
        GraphObject::Category::MISC,
        "camera",
        "Virtual camera which corresponds to a given videoprojector.");

    _objectBook["filter"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Filter>(root)); },
        GraphObject::Category::FILTER,
        "filter",
        "Filter applied to textures. The default filter allows for standard image manipulation, the user can set his own GLSL shader.",
        true);

    _objectBook["filter_black_level"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<FilterBlackLevel>(root)); },
        GraphObject::Category::FILTER,
        "black level filter",
        "Black level filter, which sets the black for an input source higher than 0 to allow for blending in dark areas.",
        true);

    _objectBook["filter_color_curves"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<FilterColorCurves>(root)); },
        GraphObject::Category::FILTER,
        "color curves filter",
        "Color curves filter, which applies color transformation based on user-defined RGB curves.",
        true);

    _objectBook["filter_custom"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<FilterCustom>(root)); },
        GraphObject::Category::FILTER,
        "custom filter",
        "Custom filter, which can take a GLSL fragment shader source to process the input texture(s).",
        true);

    _objectBook["geometry"] = Page(
        [&](RootObject* root) {
            if (_scene)
                return std::dynamic_pointer_cast<GraphObject>(_scene->getRenderer()->createGeometry(root));
            else
                return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Geometry>(root));
        },
        GraphObject::Category::MISC,
        "Geometry",
        "Intermediary object holding vertices, UV and normal coordinates of a projection surface.");

    _objectBook["image"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root)); },
        GraphObject::Category::IMAGE,
        "image",
        "Static image read from a file.",
        true);

    _objectBook["image_list"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_List>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "images from a list",
        "Static images read from a directory.",
        true);

#if HAVE_LINUX
    _objectBook["image_v4l2"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_V4L2>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "Video4Linux2 input device",
        "Image object reading frames from a Video4Linux2 compatible input.",
        true);
#endif

    _objectBook["image_ffmpeg"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_FFmpeg>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video",
        "Image object reading frames from a video file.",
        true);

#if HAVE_GPHOTO and HAVE_OPENCV
    _objectBook["image_gphoto"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_GPhoto>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "digital camera",
        "Image object reading from from a GPhoto2 compatible camera.",
        true);
#endif

#if HAVE_SH4LT
    _objectBook["image_sh4lt"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_Sh4lt>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video through Sh4lt (shared memory)",
        "Image object reading frames from a Sh4lt shared memory.",
        true);
#endif

#if HAVE_SHMDATA
    _objectBook["image_ndi"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_NDI>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video through NDI",
        "Image object reading frames from a NDI server.",
        true);

    _objectBook["image_shmdata"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_Shmdata>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video through memory shared with shmdata",
        "Image object reading frames from a shmdata shared memory.",
        true);
#endif

#if HAVE_OPENCV
    _objectBook["image_opencv"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image_OpenCV>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Image>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "camera through opencv",
        "Image object reading frames from a OpenCV compatible camera.",
        true);
#endif

    _objectBook["mesh"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Mesh>(root)); },
        GraphObject::Category::MESH,
        "mesh from obj file",
        "Mesh (vertices and UVs) describing a projection surface, read from a .obj file.",
        true);

#if HAVE_SHMDATA
    _objectBook["mesh_shmdata"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Mesh_Shmdata>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Mesh>(root));
            return object;
        },
        GraphObject::Category::MESH,
        "mesh through shared memory with shmdata",
        "Mesh object reading data from a Shmdata shared memory.",
        true);
#endif

    _objectBook["sink"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Sink>(root)); },
        GraphObject::Category::MISC,
        "sink a texture to a host buffer",
        "Get the texture content to a host buffer. Only used internally.");

#if HAVE_SH4LT
    _objectBook["sink_sh4lt"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Sink_Sh4lt>(root)); },
        GraphObject::Category::MISC,
        "sink a texture to a Sh4lt",
        "Outputs texture to a Sh4lt shared memory.");

    _objectBook["sink_sh4lt_encoded"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Sink_Sh4lt_Encoded>(root)); },
        GraphObject::Category::MISC,
        "sink a texture as an encoded video to a Sh4lt shared memory",
        "Outputs texture as a compressed frame to a Sh4lt shared memory.");
#endif

#if HAVE_SHMDATA
    _objectBook["sink_shmdata"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Sink_Shmdata>(root)); },
        GraphObject::Category::MISC,
        "sink a texture to shmdata file",
        "Outputs connected texture to a Shmdata shared memory.");

    _objectBook["sink_shmdata_encoded"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Sink_Shmdata_Encoded>(root)); },
        GraphObject::Category::MISC,
        "sink a texture as an encoded video to shmdata file",
        "Outputs texture as a compressed frame to a Shmdata shared memory.");
#endif

    _objectBook["object"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Object>(root)); },
        GraphObject::Category::MISC,
        "object",
        "Utility class used for specify which image is mapped onto which mesh.",
        true);

#if HAVE_PYTHON
    _objectBook["python"] = Page(
        [&](RootObject* root) {
            if (!root || (_scene && !_scene->isMaster()))
                return std::shared_ptr<GraphObject>(nullptr);
            return std::dynamic_pointer_cast<GraphObject>(std::make_shared<PythonEmbedded>(root));
        },
        GraphObject::Category::MISC,
        "python",
        "Allows for controlling Splash through a Python script.");
#endif

    _objectBook["queue"] = Page(
        [&](RootObject* root) {
            std::shared_ptr<GraphObject> object;
            if (!_scene)
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<Queue>(root));
            else
                object = std::dynamic_pointer_cast<GraphObject>(std::make_shared<QueueSurrogate>(root));
            return object;
        },
        GraphObject::Category::IMAGE,
        "video queue",
        "Allows for creating a timed playlist of image sources.",
        true);

    _objectBook["texture_image"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Texture_Image>(root)); },
        GraphObject::Category::TEXTURE,
        "texture image",
        "Texture object created from an Image object.",
        true);

    _objectBook["virtual_probe"] = Page(
        [&](RootObject* root) {
            if (!_scene)
                return std::dynamic_pointer_cast<GraphObject>(std::make_shared<VirtualProbe>(nullptr));
            else
                return std::dynamic_pointer_cast<GraphObject>(std::make_shared<VirtualProbe>(root));
        },
        GraphObject::Category::MISC,
        "virtual probe to simulate a virtual projection surface",
        "Virtual screen used to simulate a virtual projection surface.",
        true);

    _objectBook["warp"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Warp>(root)); },
        GraphObject::Category::MISC,
        "warp",
        "Warping object, allows for deforming the output of a Camera.");

    _objectBook["window"] = Page([&](RootObject* root) { return std::dynamic_pointer_cast<GraphObject>(std::make_shared<Window>(root)); },
        GraphObject::Category::MISC,
        "window",
        "Window object, set to be shown on one or multiple physical outputs.");
}

} // namespace Splash
