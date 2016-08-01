#include "./controller.h"

#include "./scene.h"

using namespace std;

namespace Splash {

/*************/
vector<string> ControllerObject::getObjectNames() const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};

    vector<string> objNames;
    
    for (auto& o : scene->_objects)
    {
        if (!o.second->getSavable())
            continue;
        objNames.push_back(o.first);
    }
    
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        objNames.push_back(o.first);
    }
    
    return objNames;
}

/*************/
Values ControllerObject::getObjectAttributeDescription(const string& name, const string& attr) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};
    else
        return scene->getAttributeDescriptionFromObject(name, attr);
}

/*************/
Values ControllerObject::getObjectAttribute(const string& name, const string& attr) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};
    else
        return scene->getAttributeFromObject(name, attr);
}

/*************/
unordered_map<string, Values> ControllerObject::getObjectAttributes(const string& name) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};

    auto objectIt = scene->_objects.find(name);
    if (objectIt == scene->_objects.end())
        objectIt = scene->_ghostObjects.find(name);
    if (objectIt == scene->_objects.end())
        return {};

    return objectIt->second->getAttributes();
}

/*************/
unordered_map<string, vector<string>> ControllerObject::getObjectLinks() const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};

    auto links = unordered_map<string, vector<string>>();

    for (auto& o : scene->_objects)
    {
        if (!o.second->getSavable())
            continue;
        links[o.first] = vector<string>();
        auto linkedObjects = o.second->getLinkedObjects();
        for (auto& link : linkedObjects)
        {
            links[o.first].push_back(link->getName());
        }
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        links[o.first] = vector<string>();
        auto linkedObjects = o.second->getLinkedObjects();
        for (auto& link : linkedObjects)
            links[o.first].push_back(link->getName());
    }

    return links;
}

/*************/
map<string, string> ControllerObject::getObjectTypes() const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};

    auto types = map<string, string>();

    for (auto& o : scene->_objects)
    {
        if (!o.second->getSavable())
            continue;
        types[o.first] = o.second->getType();
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        types[o.first] = o.second->getType();
    }

    return types;
}

/*************/
list<shared_ptr<BaseObject>> ControllerObject::getObjectsOfType(const string& type) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};

    auto objects = list<shared_ptr<BaseObject>>();
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == type)
            objects.push_back(obj.second);
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == type)
            objects.push_back(obj.second);

    return objects;
}

/*************/
void ControllerObject::setGlobal(const string& name, const Values& values) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return;

    scene->sendMessageToWorld(name, values);
}

/*************/
void ControllerObject::setObject(const string& name, const string& attr, const Values& values) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return;

    auto message = values;
    message.push_front(attr);
    message.push_front(name);
    scene->sendMessageToWorld("sendAll", message);
}

/*************/
void ControllerObject::setObjectsOfType(const string& type, const string& attr, const Values& values) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return;

    for (auto& obj : scene->_objects)
        if (obj.second->getType() == type)
            obj.second->setAttribute(attr, values);
    
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == type)
        {
            obj.second->setAttribute(attr, values);
            auto msg = values;
            msg.push_front(attr);
            msg.push_front(obj.first);
            scene->sendMessageToWorld("sendAll", msg);
        }
}

} // end of namespace
