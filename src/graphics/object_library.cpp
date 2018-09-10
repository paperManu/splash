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
            filepath = string(DATADIR) + filepath;
#if HAVE_OSX
        else if (ifstream("../Resources/" + filepath, ios::in | ios::binary))
            filepath = "../Resources/" + filepath;
#endif
        else
        {
            Log::get() << Log::WARNING << "Camera::" << __FUNCTION__ << " - File " << filename << " does not seem to be readable." << Log::endl;
            return false;
        }
    }

    auto mesh = make_shared<Mesh>(_root);
    mesh->setName(name);
    mesh->setAttribute("file", {filepath});
    _meshes.emplace(make_pair(name, mesh));

    auto obj = make_unique<Object>(_root);
    obj->setName(name);

    // We create the geometry manually for it not to be registered in the root
    auto geometry = make_shared<Geometry>(_root);
    _geometries.emplace(make_pair(name, geometry));

    geometry->linkTo(mesh);
    obj->linkTo(geometry);

    _library[name] = move(obj);
    return true;
}

/*************/
Object* ObjectLibrary::getModel(const string& name)
{
    auto objectIt = _library.find(name);
    if (objectIt == _library.end())
        return nullptr;
    return objectIt->second.get();
}

} // namespace Splash
