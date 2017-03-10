#include "./controller.h"

#include <algorithm>

#include "./factory.h"
#include "./scene.h"

using namespace std;

namespace Splash
{

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
unordered_map<string, vector<string>> ControllerObject::getObjectReversedLinks() const
{
    auto links = getObjectLinks();
    auto reversedLinks = unordered_map<string, vector<string>>();

    for (auto& l : links)
    {
        auto parent = l.first;
        auto children = l.second;
        for (auto& child : children)
        {
            auto childIt = reversedLinks.find(child);
            if (childIt == reversedLinks.end())
            {
                reversedLinks[child] = {parent};
            }
            else
            {
                auto parentIt = find(childIt->second.begin(), childIt->second.end(), parent);
                if (parentIt == childIt->second.end())
                    childIt->second.push_back(parent);
            }
        }
    }

    return reversedLinks;
}

/*************/
string ControllerObject::getShortDescription(const string& type) const
{
    Factory factory;
    return factory.getShortDescription(type);
}

/*************/
string ControllerObject::getDescription(const string& type) const
{
    Factory factory;
    return factory.getDescription(type);
}

/*************/
vector<string> ControllerObject::getTypesFromCategory(const BaseObject::Category& category) const
{
    Factory factory;
    return factory.getObjectsOfCategory(category);
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
        auto type = o.second->getRemoteType();
        types[o.first] = type.empty() ? o.second->getType() : type;
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        auto type = o.second->getRemoteType();
        types[o.first] = type.empty() ? o.second->getType() : type;
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
        if (obj.second->getType() == type || type == "")
            objects.push_back(obj.second);
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == type || type == "")
            objects.push_back(obj.second);

    return objects;
}

/*************/
void ControllerObject::sendBuffer(const std::string& name, const std::shared_ptr<SerializedObject> buffer) const
{
    auto root = _root.lock();
    root->sendBuffer(name, buffer);
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
Values ControllerObject::getGlobal(const string& attr) const
{
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());
    if (!scene)
        return {};

    // Do not lock permanently if no answer is received, if the attribute does not exist for example
    // We wait for no more than 10000us
    auto answer = scene->sendMessageToWorldWithAnswer("getWorldAttribute", {attr}, 10000);
    if (!answer.empty())
        answer.pop_front();
    return answer;
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

/*************/
void ControllerObject::setUserInputCallback(const UserInput::State& state, std::function<void(const UserInput::State&)> cb) const
{
    UserInput::setCallback(state, cb);
}

} // end of namespace
