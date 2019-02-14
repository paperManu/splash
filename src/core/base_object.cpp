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
void BaseObject::addRecurringTask(const string& name, const function<void()>& task)
{
    unique_lock<mutex> lock(_recurringTaskMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        Log::get() << Log::WARNING << "RootObject::" << __FUNCTION__ << " - A recurring task cannot add another recurring task" << Log::endl;
        return;
    }

    auto recurringTask = _recurringTasks.find(name);
    if (recurringTask == _recurringTasks.end())
        _recurringTasks.emplace(make_pair(name, task));
    else
        recurringTask->second = task;

    return;
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
    bool attribResult = attribFunction->second(args);

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
void BaseObject::removeRecurringTask(const string& name)
{
    unique_lock<mutex> lock(_recurringTaskMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        Log::get() << Log::WARNING << "RootObject::" << __FUNCTION__ << " - A recurring task cannot remove a recurring task" << Log::endl;
        return;
    }

    auto recurringTask = _recurringTasks.find(name);
    if (recurringTask != _recurringTasks.end())
        _recurringTasks.erase(recurringTask);
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
    unique_lock<recursive_mutex> lock(_taskMutex);
    decltype(_taskQueue) tasks;
    std::swap(tasks, _taskQueue);
    _taskQueue.clear();
    lock.unlock();

    for (const auto& task : tasks)
        task();

    unique_lock<mutex> lockRecurrsiveTasks(_recurringTaskMutex);
    for (const auto& task : _recurringTasks)
        task.second();
}
} // namespace Splash
