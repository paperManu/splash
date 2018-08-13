#include "./core/root_object.h"

#include <stdexcept>

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
bool RootObject::addTreeCommand(const string& root, Command cmd, const Values& args)
{
    if (!_tree.hasBranchAt("/" + root))
        return false;
    assert(_tree.hasBranchAt("/" + root + "/commands"));

    auto timestampAsStr = to_string(Timer::get().getTime());
    auto path = "/" + root + "/commands/" + timestampAsStr + "_";
    uint32_t cmdIndex = 0;
    while (_tree.hasLeafAt(path + to_string(cmdIndex)))
        cmdIndex++;
    path += "_" + to_string(cmdIndex);
    _tree.createLeafAt(path);
    _tree.setValueForLeafAt(path, {static_cast<int>(cmd), args});

    return true;
}

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
void RootObject::executeTreeCommands()
{
    const std::string path = "/" + _name + "/commands";

    auto commandIds = _tree.getBranchAt(path)->getLeafList();
    commandIds.sort(); // We want the commands by timing order
    for (const auto& commandId : commandIds)
    {
        Value value;
        _tree.getValueForLeafAt(path + "/" + commandId, value);
        auto command = value.as<Values>();
        assert(command.size() == 2);
        auto type = static_cast<Command>(command[0].as<int>());
        auto args = command[1].as<Values>();

        switch (type)
        {
        default:
        {
            assert(false);
            break;
        }
        case Command::callObject:
        {
            assert(args.size() == 3);
            auto objectName = args[0].as<string>();
            auto attrName = args[1].as<string>();
            auto params = args[2].as<Values>();

            auto objectIt = _objects.find(objectName);
            if (objectIt != _objects.end())
                objectIt->second->setAttribute(attrName, params);
            break;
        }
        case Command::callRoot:
        {
            assert(args.size() == 2);
            auto attrName = args[0].as<string>();
            auto params = args[1].as<Values>();
            setAttribute(attrName, params);
            break;
        }
        }
    }

    _tree.removeBranchAt(path);
    _tree.createBranchAt(path);
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
    // Update logs
    auto logs = Log::get().getNewLogs();
    for (auto& log : logs)
    {
        auto timestampAsStr = to_string(std::get<0>(log));
        auto path = "/" + _name + "/logs/" + to_string(std::get<0>(log)) + "_";
        uint32_t logIndex = 0;
        while (_tree.hasLeafAt(path + "_" + to_string(logIndex)))
            logIndex++;
        path = path + "_" + to_string(logIndex);
        _tree.createLeafAt(path);
        _tree.setValueForLeafAt(path, {std::get<1>(log), static_cast<int>(std::get<2>(log))});
    }

    // Update durations
    auto& durationMap = Timer::get().getDurationMap();
    for (auto& d : durationMap)
    {
        string path = "/" + _name + "/durations/" + d.first;
        if (!_tree.hasLeafAt(path))
            if (!_tree.createLeafAt(path))
                continue;
        _tree.setValueForLeafAt(path, {static_cast<int>(d.second)});
    }

    // Update the Root object attributes
    auto attributePath = string("/" + _name + "/attributes");
    auto attributesBranch = _tree.getBranchAt(attributePath);
    assert(attributesBranch != nullptr);

    auto leafList = attributesBranch->getLeafList();
    for (const auto& leafName : leafList)
    {
        auto attribIt = _attribFunctions.find(leafName);
        if (attribIt == _attribFunctions.end())
            continue;
        Values attribValue = attribIt->second();
        _tree.setValueForLeafAt(attributePath + "/" + leafName, attribValue);
    }

    // Update the GraphObjects attributes
    auto objectsPath = string("/" + _name + "/objects");
    auto objectsBranch = _tree.getBranchAt(objectsPath);
    assert(objectsBranch != nullptr);

    auto objectLeafList = objectsBranch->getBranchList();
    for (const auto& objectName : objectLeafList)
    {
        auto objectIt = _objects.find(objectName);
        if (objectIt == _objects.end())
            continue;
        auto object = objectIt->second;

        attributePath = string("/" + _name + "/objects/" + objectName + "/attributes");
        attributesBranch = _tree.getBranchAt(attributePath);
        if (!attributesBranch)
            continue;

        leafList = attributesBranch->getLeafList();
        for (const auto& leafName : leafList)
        {
            Values attribValue;
            object->getAttribute(leafName, attribValue);
            _tree.setValueForLeafAt(attributePath + "/" + leafName, attribValue);
        }
    }

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
    _tree.createBranchAt("/world/attributes");
    _tree.createBranchAt("/world/commands");
    _tree.createBranchAt("/world/durations");
    _tree.createBranchAt("/world/logs");
    _tree.createBranchAt("/world/objects");

    // Clear the seed list, all these leaves being automatically added to all root objets
    _tree.clearSeedList();

    // Create the leaves for all of the root attributes
    // This is done in the main loop to grab all created attributes
    addTask([this]() {
        auto path = "/" + _name + "/attributes/";
        for (const auto& attribute : _attribFunctions)
        {
            if (!attribute.second.hasGetter())
                continue;
            auto attributeName = attribute.first;
            auto leafPath = path + attributeName;
            if (_tree.hasLeafAt(leafPath))
                continue;
            if (!_tree.createLeafAt(leafPath))
                throw runtime_error("Error while adding a leaf at path " + leafPath);

            auto leaf = _tree.getLeafAt(leafPath);
            _treeCallbackIds[attributeName] = leaf->addCallback([=](const Value& value, const chrono::system_clock::time_point& /*timestamp*/) {
                auto attribIt = _attribFunctions.find(attributeName);
                if (attribIt == _attribFunctions.end())
                    return;
                if (value == attribIt->second())
                    return;
                setAttribute(attributeName, value.as<Values>());
            });
        }
    });
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
