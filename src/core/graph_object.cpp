#include "./core/graph_object.h"

#include <algorithm>

using namespace std;

namespace Splash
{

/*************/
Attribute& GraphObject::operator[](const string& attr)
{
    auto attribFunction = _attribFunctions.find(attr);
    return attribFunction->second;
}

/*************/
void GraphObject::linkToParent(GraphObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt == _parents.end())
        _parents.push_back(obj);
    return;
}

/*************/
void GraphObject::unlinkFromParent(GraphObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt != _parents.end())
        _parents.erase(parentIt);
    return;
}

/*************/
bool GraphObject::linkTo(const shared_ptr<GraphObject>& obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const weak_ptr<GraphObject>& o) {
        auto object = o.lock();
        if (!object)
            return false;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt == _linkedObjects.end())
    {
        _linkedObjects.push_back(obj);
        obj->linkToParent(this);
        return true;
    }
    return false;
}

/*************/
void GraphObject::unlinkFrom(const shared_ptr<GraphObject>& obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const weak_ptr<GraphObject>& o) {
        auto object = o.lock();
        if (!object)
            return false;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt != _linkedObjects.end())
    {
        _linkedObjects.erase(objectIt);
        obj->unlinkFromParent(this);
    }
}

/*************/
Json::Value GraphObject::getConfigurationAsJson() const
{
    Json::Value root;
    if (_remoteType == "")
        root["type"] = _type;
    else
        root["type"] = _remoteType;

    for (auto& attr : _attribFunctions)
    {
        Values values;
        if (getAttribute(attr.first, values) == false || values.size() == 0)
            continue;

        Json::Value jsValue;
        jsValue = getValuesAsJson(values);
        root[attr.first] = jsValue;
    }
    return root;
}

/*************/
unordered_map<string, Values> GraphObject::getDistantAttributes() const
{
    unordered_map<string, Values> attribs;
    for (auto& attr : _attribFunctions)
    {
        if (!attr.second.doUpdateDistant())
            continue;

        Values values;
        if (getAttribute(attr.first, values, false, true) == false || values.size() == 0)
            continue;

        attribs[attr.first] = values;
    }

    return attribs;
}

/*************/
bool GraphObject::setRenderingPriority(Priority priority)
{
    if (priority < Priority::PRE_CAMERA || priority >= Priority::POST_WINDOW)
        return false;
    _renderingPriority = priority;
    return true;
}

/*************/
CallbackHandle GraphObject::registerCallback(const string& attr, Attribute::Callback cb)
{
    auto attribute = _attribFunctions.find(attr);
    if (attribute == _attribFunctions.end())
        return CallbackHandle();

    return attribute->second.registerCallback(shared_from_this(), cb);
}

/*************/
bool GraphObject::unregisterCallback(const CallbackHandle& handle)
{
    if (!handle)
        return false;

    auto attribute = _attribFunctions.find(handle.getAttribute());
    if (attribute == _attribFunctions.end())
        return false;

    return attribute->second.unregisterCallback(handle);
}

/*************/
void GraphObject::registerAttributes()
{
    addAttribute("alias",
        [&](const Values& args) {
            auto alias = args[0].as<string>();
            setAlias(alias);
            return true;
        },
        [&]() -> Values { return {getAlias()}; },
        {'s'});
    setAttributeDescription("alias", "Alias name");

    addAttribute("setSavable",
        [&](const Values& args) {
            auto savable = args[0].as<bool>();
            setSavable(savable);
            return true;
        },
        {'n'});

    addAttribute("priorityShift",
        [&](const Values& args) {
            _priorityShift = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_priorityShift}; },
        {'n'});
    setAttributeDescription("priorityShift",
        "Shift to the default rendering priority value, for those cases where two objects should be rendered in a specific order. Higher value means lower priority");

    addAttribute("switchLock",
        [&](const Values& args) {
            auto attribIterator = _attribFunctions.find(args[0].as<string>());
            if (attribIterator == _attribFunctions.end())
                return false;

            string status;
            auto& attribFunctor = attribIterator->second;
            if (attribFunctor.isLocked())
            {
                status = "Unlocked";
                attribFunctor.unlock();
            }
            else
            {
                status = "Locked";
                attribFunctor.lock();
            }

            Log::get() << Log::MESSAGE << _name << "~~" << args[0].as<string>() << " - " << status << Log::endl;
            return true;
        },
        {'s'});
}

} // namespace Splash
