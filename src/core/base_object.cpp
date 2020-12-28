#include "./core/base_object.h"

#include <algorithm>

#include "./utils/log.h"

namespace Splash
{

/*************/
void BaseObject::addTask(const std::function<void()>& task)
{
    std::lock_guard<std::recursive_mutex> lock(_taskMutex);
    _taskQueue.push_back(task);
}

/*************/
void BaseObject::addPeriodicTask(const std::string& name, const std::function<void()>& task, uint32_t period)
{
    std::unique_lock<std::mutex> lock(_periodicTaskMutex, std::try_to_lock);
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
BaseObject::SetAttrStatus BaseObject::setAttribute(const std::string& attrib, const Values& args)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attribFunction = _attribFunctions.find(attrib);
    const bool attribNotPresent = (attribFunction == _attribFunctions.end());

    if (attribNotPresent)
    {
        const auto result = _attribFunctions.emplace(attrib, Attribute(attrib));
        assert(result.second);
        attribFunction = result.first;
    }

    // If the attribute is not a default one, signify that the parameters
    // have been updated
    if (!attribFunction->second.isDefault())
        _updatedParams = true;

    // If the attribute is not a default one, but does not have a setter set,
    // there is no need to try to set a new value
    if (!attribFunction->second.isDefault() && !attribFunction->second.hasSetter())
        return SetAttrStatus::no_setter;

    // Otherwise try setting the value. If this fails, something is wrong
    // with the setter function
    if (!attribFunction->second(args))
        return SetAttrStatus::failure;

    return SetAttrStatus::success;
}

/*************/
bool BaseObject::getAttribute(const std::string& attrib, Values& args) const
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    const auto attribFunction = _attribFunctions.find(attrib);
    if (attribFunction == _attribFunctions.end())
    {
        args.clear();
        return false;
    }

    args = attribFunction->second();
    if (args.empty())
        return false;

    return true;
}

/*************/
std::optional<Values> BaseObject::getAttribute(const std::string& attrib) const
{
    Values value;
    if (!getAttribute(attrib, value))
        return {};
    return value;
}

/*************/
std::vector<std::string> BaseObject::getAttributesList() const
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    std::vector<std::string> attributeNames;
    for (const auto& [name, attribute] : _attribFunctions)
        attributeNames.push_back(name);
    return attributeNames;
}

/*************/
std::string BaseObject::getAttributeDescription(const std::string& name) const
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        return attr->second.getDescription();
    else
        return {};
}

/*************/
Values BaseObject::getAttributesDescriptions() const
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    Values descriptions;
    for (const auto& attr : _attribFunctions)
        descriptions.push_back(Values({attr.first, attr.second.getDescription(), attr.second.getArgsTypes()}));
    return descriptions;
}

/*************/
Attribute::Sync BaseObject::getAttributeSyncMethod(const std::string& name)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        return attr->second.getSyncMethod();
    else
        return Attribute::Sync::auto_sync;
}

/*************/
void BaseObject::runAsyncTask(const std::function<void(void)>& func)
{
    std::lock_guard<std::mutex> lockTasks(_asyncTaskMutex);
    auto taskId = _nextAsyncTaskId++;
    _asyncTasks[taskId] = std::async(std::launch::async, [this, func, taskId]() -> void {
        func();
        addTask([this, taskId]() -> void {
            std::lock_guard<std::mutex> lockTasks(_asyncTaskMutex);
            _asyncTasks.erase(taskId);
        });
    });
}

/*************/
Attribute& BaseObject::addAttribute(
    const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    _attribFunctions[name] = Attribute(name, set, get, types);
    _attribFunctions[name].setObjectName(_name);
    return _attribFunctions[name];
}

/*************/
Attribute& BaseObject::addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    _attribFunctions[name] = Attribute(name, set, nullptr, types);
    _attribFunctions[name].setObjectName(_name);
    return _attribFunctions[name];
}

/*************/
Attribute& BaseObject::addAttribute(const std::string& name, const std::function<const Values()>& get)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    _attribFunctions[name] = Attribute(name, get);
    _attribFunctions[name].setObjectName(_name);
    return _attribFunctions[name];
}

/*************/
bool BaseObject::hasAttribute(const std::string& name) const
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    if (_attribFunctions.find(name) == _attribFunctions.end())
        return false;
    return true;
}

/*************/
void BaseObject::setAttributeDescription(const std::string& name, const std::string& description)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
    {
        attr->second.setDescription(description);
    }
}

/*************/
void BaseObject::setAttributeSyncMethod(const std::string& name, const Attribute::Sync& method)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        attr->second.setSyncMethod(method);
}

/*************/
void BaseObject::removeAttribute(const std::string& name)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        _attribFunctions.erase(attr);
}

/*************/
void BaseObject::removePeriodicTask(const std::string& name)
{
    std::unique_lock<std::mutex> lock(_periodicTaskMutex, std::try_to_lock);
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
CallbackHandle BaseObject::registerCallback(const std::string& attr, Attribute::Callback cb)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
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

    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attribute = _attribFunctions.find(handle.getAttribute());
    if (attribute == _attribFunctions.end())
        return false;

    return attribute->second.unregisterCallback(handle);
}

/*************/
void BaseObject::runTasks()
{
    std::unique_lock<std::recursive_mutex> lock(_taskMutex);
    decltype(_taskQueue) tasks;
    std::swap(tasks, _taskQueue);
    _taskQueue.clear();
    lock.unlock();

    for (const auto& task : tasks)
        task();

    std::unique_lock<std::mutex> lockRecurrsiveTasks(_periodicTaskMutex);
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
