#include "./core/root_object.h"

#include "./core/buffer_object.h"
#include "./core/serializer.h"

using namespace std;

namespace Splash
{

/**************/
RootObject::RootObject()
    : _factory(unique_ptr<Factory>(new Factory(this)))
{
    registerAttributes();
    initializeTree();
}

/*************/
RootObject::~RootObject() {}

/*************/
weak_ptr<GraphObject> RootObject::createObject(const string& type, const string& name)
{
    lock_guard<recursive_mutex> registerLock(_objectsMutex);

    auto object = getObject(name);
    if (object)
    {
        auto objectType = object->getType();
        if (objectType != type)
        {
            Log::get() << Log::WARNING << "RootObject::" << __FUNCTION__ << " - Object with name " << name << " already exists, but with a different type (" << objectType
                       << " instead of " << type << ")" << Log::endl;
            return {};
        }

        return object;
    }
    else
    {
        auto object = _factory->create(type);
        if (!object)
            return {};

        object->setName(name);
        object->setSavable(false);
        _objects[name] = object;
        return object;
    }
}

/*************/
void RootObject::disposeObject(const string& name)
{
    addTask([=]() {
        lock_guard<recursive_mutex> registerLock(_objectsMutex);
        auto objectIt = _objects.find(name);
        if (objectIt != _objects.end() && objectIt->second.use_count() == 1)
            _objects.erase(objectIt);
    });
}

/*************/
shared_ptr<GraphObject> RootObject::getObject(const string& name)
{
    lock_guard<recursive_mutex> registerLock(_objectsMutex);

    auto objectIt = _objects.find(name);
    if (objectIt != _objects.end())
        return objectIt->second;

    return {nullptr};
}

/*************/
bool RootObject::set(const string& name, const string& attrib, const Values& args, bool async)
{
    if (name == _name || name == SPLASH_ALL_PEERS)
        return setAttribute(attrib, args);

    auto object = getObject(name);
    if (object && object->getAttributeSyncMethod(attrib) == Attribute::Sync::force_sync)
        async = false;

    if (async)
    {
        addTask([=]() {
            auto object = getObject(name);
            if (object)
                object->setAttribute(attrib, args);
        });
    }
    else
    {
        if (object)
            return object->setAttribute(attrib, args);
        else
            return false;
    }

    return true;
}

/*************/
void RootObject::setFromSerializedObject(const string& name, shared_ptr<SerializedObject> obj)
{
    auto object = getObject(name);
    if (object)
    {
        auto objectAsBuffer = dynamic_pointer_cast<BufferObject>(object);
        if (objectAsBuffer)
            objectAsBuffer->setSerializedObject(move(obj));
    }
    else
    {
        handleSerializedObject(name, move(obj));
    }
}

/*************/
void RootObject::signalBufferObjectUpdated()
{
    unique_lock<mutex> lockCondition(_bufferObjectUpdatedMutex);

    _bufferObjectUpdated = true;
    // Only a single buffer has to wave for update at a time
    if (!_bufferObjectSingleMutex.try_lock())
        return;

    _bufferObjectUpdatedCondition.notify_all();
    _bufferObjectSingleMutex.unlock();
}

/*************/
bool RootObject::waitSignalBufferObjectUpdated(uint64_t timeout)
{
    unique_lock<mutex> lockCondition(_bufferObjectUpdatedMutex);

    auto bufferWasUpdated = _bufferObjectUpdated;

    if (bufferWasUpdated == true)
    {
        _bufferObjectUpdated = false;
        return true;
    }
    else if (timeout != 0)
    {
        auto status = _bufferObjectUpdatedCondition.wait_for(lockCondition, chrono::microseconds(timeout));
        _bufferObjectUpdated = false;
        return (status == cv_status::no_timeout);
    }
    else
    {
        _bufferObjectUpdatedCondition.wait(lockCondition);
        return true;
    }
}

/*************/
bool RootObject::handleSerializedObject(const string& name, shared_ptr<SerializedObject> obj)
{
    if (name == "_tree")
    {
        auto dataPtr = reinterpret_cast<uint8_t*>(obj->data());
        auto serializedSeeds = vector<uint8_t>(dataPtr, dataPtr + obj->size());
        auto seeds = Serial::deserialize<list<Tree::Seed>>(serializedSeeds);
        _tree.addSeedsToQueue(seeds);

        return true;
    }

    return false;
}

/*************/
void RootObject::addRecurringTask(const string& name, const function<void()>& task)
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
void RootObject::propagateTree()
{
    auto treeSeeds = _tree.getSeedList();
    vector<uint8_t> serializedSeeds;
    Serial::serialize(treeSeeds, serializedSeeds);
    auto dataPtr = reinterpret_cast<char*>(serializedSeeds.data());
    _link->sendBuffer("_tree", make_shared<SerializedObject>(dataPtr, dataPtr + serializedSeeds.size()));
}

/*************/
void RootObject::removeRecurringTask(const string& name)
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
void RootObject::registerAttributes()
{
    addAttribute("answerMessage", [&](const Values& args) {
        if (args.size() == 0 || args[0].as<string>() != _answerExpected)
            return false;
        unique_lock<mutex> conditionLock(_conditionMutex);
        _lastAnswerReceived = args;
        _answerCondition.notify_one();
        return true;
    });
}

/*************/
void RootObject::initializeTree()
{
    _tree.createBranchAt("/world");
    _tree.createBranchAt("/world/durations");
    _tree.createBranchAt("/world/logs");
    _tree.createLeafAt("/world/clock");
    _tree.createLeafAt("/world/master_clock");

    // Clear the seed list, all these leaves being automatically added to all root objets
    _tree.clearSeedList();
}

/*************/
void RootObject::runTasks()
{
    unique_lock<recursive_mutex> lockTasks(_taskMutex);
    decltype(_taskQueue) tasks;
    std::swap(tasks, _taskQueue);
    lockTasks.unlock();
    for (auto& task : tasks)
        task();

    unique_lock<mutex> lockRecurrsiveTasks(_recurringTaskMutex);
    for (auto& task : _recurringTasks)
        task.second();
}

/*************/
Values RootObject::sendMessageWithAnswer(const string& name, const string& attribute, const Values& message, const unsigned long long timeout)
{
    if (_link == nullptr)
        return {};

    lock_guard<mutex> lock(_answerMutex);
    _answerExpected = attribute;

    unique_lock<mutex> conditionLock(_conditionMutex);
    _link->sendMessage(name, attribute, message);

    auto cvStatus = cv_status::no_timeout;
    if (timeout == 0ull)
        _answerCondition.wait(conditionLock);
    else
        cvStatus = _answerCondition.wait_for(conditionLock, chrono::microseconds(timeout));

    _answerExpected = "";

    if (cv_status::no_timeout == cvStatus)
        return _lastAnswerReceived;
    else
        return {};
}

} // namespace Splash
