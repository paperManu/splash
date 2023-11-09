#include "./graphics/object_library.h"

#include <fstream>
#include <utility>

#include "./core/scene.h"

namespace Splash
{

/*************/
ObjectLibrary::ObjectLibrary(Scene* scene)
    : _scene(scene)
{
    if (scene != nullptr)
        _renderer = scene->getRenderer();
}

/*************/
bool ObjectLibrary::loadModel(const std::string& name, const std::string& filename)
{
    if (_library.find(name) != _library.end())
        return false;

    std::string filepath = filename;

    if (!std::ifstream(filepath, std::ios::in | std::ios::binary))
    {
        if (std::ifstream(std::string(DATADIR) + filepath, std::ios::in | std::ios::binary))
        {
            filepath = std::string(DATADIR) + filepath;
        }
        else
        {
            Log::get() << Log::WARNING << "ObjectLibrary::" << __FUNCTION__ << " - File " << filename << " does not seem to be readable." << Log::endl;
            return false;
        }
    }

    assert(_renderer != nullptr);

    auto mesh = std::make_shared<Mesh>(_scene);
    mesh->setAttribute("file", {filepath});

    // We create the geometry manually for it not to be registered in the root
    auto geometry = _renderer->createGeometry(_scene);
    geometry->setMesh(mesh);

    auto obj = std::make_unique<Object>(_scene);
    obj->addGeometry(geometry);

    _library[name] = std::move(obj);
    return true;
}

/*************/
Object* ObjectLibrary::getModel(const std::string& name)
{
    auto objectIt = _library.find(name);
    if (objectIt == _library.end())
    {
        // The default object must be created in a GL context, so it
        // can not be created in the constructor of the library
        if (!_defaultObject)
        {
            auto geometry = _renderer->createGeometry(_scene);
            _defaultObject = std::make_unique<Object>(_scene);
            _defaultObject->addGeometry(geometry);
        }

        Log::get() << Log::WARNING << "ObjectLibrary::" << __FUNCTION__ << " - No object named " << name << " in the library" << Log::endl;
        return _defaultObject.get();
    }
    return objectIt->second.get();
}

} // namespace Splash
