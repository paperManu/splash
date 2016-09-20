#include "./factory.h"

#include "./camera.h"
#if HAVE_GPHOTO
#include "./colorcalibrator.h"
#endif
#include "./controller_blender.h"
#include "./filter.h"
#include "./geometry.h"
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
#endif
#include "./link.h"
#include "./log.h"
#include "./mesh.h"
#if HAVE_SHMDATA
#include "./mesh_shmdata.h"
#endif
#include "./object.h"
#if HAVE_PYTHON
#include "./controller_pythonEmbedded.h"
#endif
#include "./queue.h"
#include "./texture.h"
#include "./texture_image.h"
#if HAVE_OSX
#include "./texture_syphon.h"
#endif
#include "./threadpool.h"
#include "./timer.h"
#include "./warp.h"
#include "./window.h"

using namespace std;

namespace Splash
{

/*************/
Factory::Factory()
{
    registerObjects();
}

/*************/
Factory::Factory(weak_ptr<RootObject> root)
{
    _root = root;
    if (!root.expired() && root.lock()->getType() == "scene")
        _isScene = true;

    registerObjects();
}

/*************/
shared_ptr<BaseObject> Factory::create(string type)
{
    // Not all object types are listed here, only those which are available to the user are
    auto page = _objectBook.find(type);
    if (page != _objectBook.end())
    {
        return page->second();
    }
    else
    {
        Log::get() << Log::WARNING << "Factory::" << __FUNCTION__ << " - Object type " << type << " does not exist" << Log::endl;
        return {nullptr};
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
void Factory::registerObjects()
{
    _objectBook["blender"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Blender>(_root)); };
    _objectBook["camera"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Camera>(_root)); };
    _objectBook["filter"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Filter>(_root)); };
    _objectBook["geometry"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Geometry>(_root)); };
    _objectBook["image"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Image>(_root)); };

#if HAVE_FFMPEG
    _objectBook["image_ffmpeg"] = [&]() {
        shared_ptr<BaseObject> object;
        if (!_isScene)
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image_FFmpeg>(_root));
        else
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image>(_root));
        return object;
    };
#endif

#if HAVE_GPHOTO
    _objectBook["image_gphoto"] = [&]() {
        shared_ptr<BaseObject> object;
        if (!_isScene)
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image_GPhoto>(_root));
        else
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image>(_root));
        return object;
    };
#endif

#if HAVE_SHMDATA
    _objectBook["image_shmdata"] = [&]() {
        shared_ptr<BaseObject> object;
        if (!_isScene)
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image_Shmdata>(_root));
        else
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image>(_root));
        return object;
    };
#endif

#if HAVE_OPENCV
    _objectBook["image_opencv"] = [&]() {
        shared_ptr<BaseObject> object;
        if (!_isScene)
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image_OpenCV>(_root));
        else
            object = dynamic_pointer_cast<BaseObject>(make_shared<Image>(_root));
        return object;
    };
#endif

    _objectBook["mesh"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Mesh>(_root)); };

#if HAVE_SHMDATA
    _objectBook["mesh_shmdata"] = [&]() {
        shared_ptr<BaseObject> object;
        if (!_isScene)
            object = dynamic_pointer_cast<BaseObject>(make_shared<Mesh_Shmdata>(_root));
        else
            object = dynamic_pointer_cast<BaseObject>(make_shared<Mesh>(_root));
        return object;
    };
#endif

    _objectBook["object"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Object>(_root)); };

#if HAVE_PYTHON
    _objectBook["python"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<PythonEmbedded>(_root)); };
#endif

    _objectBook["queue"] = [&]() {
        shared_ptr<BaseObject> object;
        if (!_isScene)
            object = dynamic_pointer_cast<BaseObject>(make_shared<Queue>(_root));
        else
            object = dynamic_pointer_cast<BaseObject>(make_shared<QueueSurrogate>(_root));
        return object;
    };

    _objectBook["texture_image"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Texture_Image>(_root)); };

#if HAVE_OSX
    _objectBook["texture_syphon"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Texture_Syphon>(_root)); };
#endif

    _objectBook["warp"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Warp>(_root)); };
    _objectBook["window"] = [&]() { return dynamic_pointer_cast<BaseObject>(make_shared<Window>(_root)); };
}

} // end of namespace
