#include "./core/base_object.h"

#include <algorithm>

#include "./utils/log.h"

using namespace std;

namespace Splash
{

/*************/
void BaseObject::addTask(const function<void()>& task)
{
    lock_guard<recursive_mutex> lock(_taskMutex);
    _taskQueue.push_back(task);
}

/*************/
bool BaseObject::setAttribute(const string& attrib, const Values& args)
{
    auto attribFunction = _attribFunctions.find(attrib);
    bool attribNotPresent = (attribFunction == _attribFunctions.end());

    if (attribNotPresent)
    {
        auto result = _attribFunctions.emplace(attrib, Attribute(attrib));
        if (!result.second)
            return false;

        attribFunction = result.first;
    }

    if (!attribFunction->second.isDefault())
        _updatedParams = true;
    bool attribResult = attribFunction->second(forward<const Values&>(args));

    return attribResult && attribNotPresent;
}

/*************/
bool BaseObject::getAttribute(const string& attrib, Values& args) const
{
    auto attribFunction = _attribFunctions.find(attrib);
    if (attribFunction == _attribFunctions.end())
    {
        args.clear();
        return false;
    }

    args = attribFunction->second();

    return true;
}

/*************/
string BaseObject::getAttributeDescription(const string& name)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        return attr->second.getDescription();
    else
        return {};
}

/*************/
Values BaseObject::getAttributesDescriptions()
{
    Values descriptions;
    for (const auto& attr : _attribFunctions)
        descriptions.push_back(Values({attr.first, attr.second.getDescription(), attr.second.getArgsTypes()}));
    return descriptions;
}

/*************/
Attribute::Sync BaseObject::getAttributeSyncMethod(const string& name)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        return attr->second.getSyncMethod();
    else
        return Attribute::Sync::no_sync;
}

/*************/
void BaseObject::runAsyncTask(const function<void(void)>& func)
{
    lock_guard<mutex> lockTasks(_asyncTaskMutex);
    _asyncTask = async(launch::async, func);
}

/*************/
Attribute& BaseObject::addAttribute(const string& name, const function<bool(const Values&)>& set, const vector<char>& types)
{
    _attribFunctions[name] = Attribute(name, set, nullptr, types);
    _attribFunctions[name].setObjectName(_name);
    return _attribFunctions[name];
}

/*************/
Attribute& BaseObject::addAttribute(const string& name, const function<bool(const Values&)>& set, const function<const Values()>& get, const vector<char>& types)
{
    _attribFunctions[name] = Attribute(name, set, get, types);
    _attribFunctions[name].setObjectName(_name);
    return _attribFunctions[name];
}

/*************/
void BaseObject::setAttributeDescription(const string& name, const string& description)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
    {
        attr->second.setDescription(description);
    }
}

/*************/
void BaseObject::setAttributeSyncMethod(const string& name, const Attribute::Sync& method)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        attr->second.setSyncMethod(method);
}

/*************/
void BaseObject::removeAttribute(const string& name)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        _attribFunctions.erase(attr);
}

/*************/
void BaseObject::setAttributeParameter(const string& name, bool savable)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        attr->second.savable(savable);
}

/*************/
CallbackHandle BaseObject::registerCallback(const string& attr, Attribute::Callback cb)
{
    auto attribute = _attribFunctions.find(attr);
    if (attribute == _attribFunctions.end())
        return CallbackHandle();

    return attribute->second.registerCallback(shared_from_this(), cb);
}

/*************/
bool BaseObject::unregisterCallback(const CallbackHandle& handle)
{
    if (!handle)
        return false;

    auto attribute = _attribFunctions.find(handle.getAttribute());
    if (attribute == _attribFunctions.end())
        return false;

    return attribute->second.unregisterCallback(handle);
}

/*************/
void BaseObject::runTasks()
{
    lock_guard<recursive_mutex> lock(_taskMutex);
    for (auto& task : _taskQueue)
        task();
    _taskQueue.clear();
}
} // namespace Splash
