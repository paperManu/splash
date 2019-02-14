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
void BaseObject::addPeriodicTask(const string& name, const function<void()>& task, uint32_t period)
{
    unique_lock<mutex> lock(_periodicTaskMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        Log::get() << Log::WARNING << "RootObject::" << __FUNCTION__ << " - A periodic task cannot add another periodic task" << Log::endl;
        return;
    }

    auto periodicTask = _periodicTasks.find(name);
    if (periodicTask == _periodicTasks.end())
    {
        _periodicTasks.emplace(make_pair(name, PeriodicTask(task, period)));
    }
    else
    {
        periodicTask->second = {task, period};
    }

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
void BaseObject::removePeriodicTask(const string& name)
{
    unique_lock<mutex> lock(_periodicTaskMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        Log::get() << Log::WARNING << "RootObject::" << __FUNCTION__ << " - A periodic task cannot remove a periodic task" << Log::endl;
        return;
    }

    auto periodicTask = _periodicTasks.find(name);
    if (periodicTask != _periodicTasks.end())
        _periodicTasks.erase(periodicTask);
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

    unique_lock<mutex> lockRecurrsiveTasks(_periodicTaskMutex);
    auto currentTime = Timer::getTime() / 1000;
    for (auto& task : _periodicTasks)
    {
        if (task.second.period == 0 || currentTime - task.second.lastCall > task.second.period)
        {
            task.second.task();
            task.second.lastCall = currentTime;
        }
    }
}
} // namespace Splash
