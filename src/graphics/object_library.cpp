#include "./graphics/object_library.h"

#include <fstream>
#include <utility>

using namespace std;

namespace Splash
{

/*************/
bool ObjectLibrary::loadModel(const string& name, const string& filename)
{
    if (_library.find(name) != _library.end())
        return false;

    string filepath = filename;

    if (!ifstream(filepath, ios::in | ios::binary))
    {
        if (ifstream(string(DATADIR) + filepath, ios::in | ios::binary))
        {
            filepath = string(DATADIR) + filepath;
        }
        else
        {
            Log::get() << Log::WARNING << "ObjectLibrary::" << __FUNCTION__ << " - File " << filename << " does not seem to be readable." << Log::endl;
            return false;
        }
    }

    auto mesh = make_shared<Mesh>(_root);
    mesh->setAttribute("file", {filepath});

    // We create the geometry manually for it not to be registered in the root
    auto geometry = make_shared<Geometry>(_root);
    geometry->setMesh(mesh);

    auto obj = make_unique<Object>(_root);
    obj->addGeometry(geometry);

    _library[name] = move(obj);
    return true;
}

/*************/
Object* ObjectLibrary::getModel(const string& name)
{
    auto objectIt = _library.find(name);
    if (objectIt == _library.end())
    {
        // The default object must be created in a GL context, so it
        // can not be created in the constructor of the library
        if (!_defaultObject)
        {
            auto geometry = make_shared<Geometry>(_root);
            _defaultObject = make_unique<Object>(_root);
            _defaultObject->addGeometry(geometry);
        }

        Log::get() << Log::WARNING << "ObjectLibrary::" << __FUNCTION__ << " - No object named " << name << " in the library" << Log::endl;
        return _defaultObject.get();
    }
    return objectIt->second.get();
}

} // namespace Splash
