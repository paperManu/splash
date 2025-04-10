#include "./core/root_object.h"

#include <stdexcept>

#include "./core/buffer_object.h"
#include "./core/constants.h"
#include "./core/serialize/serialize_uuid.h"
#include "./core/serialize/serialize_value.h"
#include "./core/serializer.h"

namespace chrono = std::chrono;

namespace Splash
{

/**************/
RootObject::RootObject()
    : _context(Context())
    , _factory(std::unique_ptr<Factory>(new Factory(this)))
{
    registerAttributes();
    initializeTree();
}

/**************/
RootObject::RootObject(Context context)
    : _context(context)
    , _factory(std::unique_ptr<Factory>(new Factory(this)))
{
    registerAttributes();
    initializeTree();
}

/*************/
bool RootObject::addTreeCommand(const std::string& root, Command cmd, const Values& args)
{
    if (!_tree.hasBranchAt("/" + root + "/commands"))
        return false;

    auto timestampAsStr = std::to_string(Timer::get().getTime());
    auto path = "/" + root + "/commands/" + timestampAsStr + "_";
    uint32_t cmdIndex = 0;
    while (_tree.hasLeafAt(path + std::to_string(cmdIndex)))
        cmdIndex++;
    path += "_" + std::to_string(cmdIndex);
    _tree.createLeafAt(path);
    _tree.setValueForLeafAt(path, {static_cast<int>(cmd), args});

    return true;
}

/*************/
std::weak_ptr<GraphObject> RootObject::createObject(const std::string& type, const std::string& name)
{
    std::lock_guard<std::recursive_mutex> registerLock(_objectsMutex);

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
void RootObject::disposeObject(const std::string& name)
{
    addTask([=]() {
        std::lock_guard<std::recursive_mutex> registerLock(_objectsMutex);
        auto objectIt = _objects.find(name);
        if (objectIt != _objects.end() && objectIt->second.use_count() == 1)
            _objects.erase(objectIt);
    });
}

/*************/
void RootObject::executeTreeCommands()
{
    const std::string path = "/" + _name + "/commands";

    auto commandIds = _tree.getLeafListAt(path);
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
            auto objectName = args[0].as<std::string>();
            auto attrName = args[1].as<std::string>();
            auto params = args[2].as<Values>();

            auto objectIt = _objects.find(objectName);
            if (objectIt != _objects.end())
                objectIt->second->setAttribute(attrName, params);
            break;
        }
        case Command::callRoot:
        {
            assert(args.size() == 2);
            auto attrName = args[0].as<std::string>();
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
std::shared_ptr<GraphObject> RootObject::getObject(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> registerLock(_objectsMutex);

    auto objectIt = _objects.find(name);
    if (objectIt != _objects.end())
        return objectIt->second;

    return {nullptr};
}

/*************/
bool RootObject::set(const std::string& name, const std::string& attrib, const Values& args, bool async)
{
    if (name == _name || name == Constants::ALL_PEERS)
        return setAttribute(attrib, args) != BaseObject::SetAttrStatus::failure;

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
            return object->setAttribute(attrib, args) != BaseObject::SetAttrStatus::failure;
        else
            return false;
    }

    return true;
}

/*************/
bool RootObject::setFromSerializedObject(const std::string& name, SerializedObject&& obj)
{
    auto object = getObject(name);
    if (object)
    {
        auto objectAsBuffer = std::dynamic_pointer_cast<BufferObject>(object);
        if (objectAsBuffer)
        {
            return objectAsBuffer->setSerializedObject(std::move(obj));
        }
    }
    else
    {
        return handleSerializedObject(name, obj);
    }

    return false;
}

/*************/
void RootObject::signalBufferObjectUpdated()
{
    std::unique_lock<std::mutex> lockCondition(_bufferObjectUpdatedMutex);

    _bufferObjectUpdated = true;
    _bufferObjectUpdatedCondition.notify_all();
}

/*************/
bool RootObject::waitSignalBufferObjectUpdated(uint64_t timeout)
{
    std::unique_lock<std::mutex> lockCondition(_bufferObjectUpdatedMutex);

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
        return (status == std::cv_status::no_timeout);
    }
    else
    {
        _bufferObjectUpdatedCondition.wait(lockCondition);
        return true;
    }
}

/*************/
bool RootObject::handleSerializedObject(const std::string& name, SerializedObject& obj)
{
    if (name == "_tree")
    {
        const auto data = obj.grabData();
        auto dataIt = data.cbegin();
        // We deserialize the buffer name, but we don't need to keep it
        Serial::detail::deserializer<std::string>(dataIt);
        auto seeds = Serial::detail::deserializer<std::list<Tree::Seed>>(dataIt);
        _tree.addSeedsToQueue(seeds);

        return true;
    }

    return false;
}

/*************/
void RootObject::updateTreeFromObjects()
{
    // Update logs
    const auto logs = Log::get().getNewLogs();
    for (auto& log : logs)
    {
        auto timestampAsStr = std::to_string(std::get<0>(log));
        auto path = "/" + _name + "/logs/" + std::to_string(std::get<0>(log)) + "_";

        // We store the logs by their index, and we know any new log
        // will have a higher index. Which is why logIndex is static.
        static uint32_t logIndex = 0;
        while (_tree.hasLeafAt(path + std::to_string(logIndex)))
            logIndex++;

        path.append(std::to_string(logIndex));
        _tree.createLeafAt(path);
        _tree.setValueForLeafAt(path, Values({std::get<1>(log), static_cast<int>(std::get<2>(log))}));
    }

    // Update durations
    const auto& durationMap = Timer::get().getDurationMap();
    for (auto& d : durationMap)
    {
        std::string path = "/" + _name + "/durations/" + d.first;
        if (!_tree.hasLeafAt(path))
            if (!_tree.createLeafAt(path))
                continue;
        _tree.setValueForLeafAt(path, Values({Value(static_cast<int>(d.second))}));
    }

    // Update the Root object attributes
    {
        const auto attributePath = std::string("/" + _name + "/attributes/");
        assert(_tree.hasBranchAt(attributePath));

        std::unique_lock<std::recursive_mutex> lock(_attribMutex);
        for (const auto& leafName : _tree.getLeafListAt(attributePath))
        {
            const auto attribIt = _attribFunctions.find(leafName);
            if (attribIt == _attribFunctions.end())
                continue;
            Values attribValue = attribIt->second();
            _tree.setValueForLeafAt(attributePath + leafName, attribValue);
        }
    }

    // Update the GraphObjects attributes
    const auto objectsPath = std::string("/" + _name + "/objects");
    assert(_tree.hasBranchAt(objectsPath));

    for (const auto& objectName : _tree.getBranchListAt(objectsPath))
    {
        const auto objectIt = _objects.find(objectName);
        if (objectIt == _objects.end())
            continue;
        const auto object = objectIt->second;

        const auto attributePath = std::string("/" + _name + "/objects/" + objectName + "/attributes/");
        const auto docPath = std::string("/" + _name + "/objects/" + objectName + "/documentation/");

        assert(_tree.hasBranchAt(attributePath));
        for (const auto& leafName : _tree.getLeafListAt(attributePath))
        {
            const auto attribValue = object->getAttribute(leafName);
            if (attribValue)
            {
                _tree.setValueForLeafAt(attributePath + leafName, attribValue.value());
            }
            else
            {
                _tree.removeLeafAt(attributePath + leafName);
                if (const auto docBranchPath = docPath + leafName; _tree.hasBranchAt(docBranchPath))
                    _tree.removeBranchAt(docBranchPath);
            }
        }
    }
}

/*************/
void RootObject::propagateTree()
{
    assert(_link);
    assert(_link->isReady());

    auto treeSeeds = _tree.getUpdateSeedList();
    if (treeSeeds.empty())
        return;
    std::vector<uint8_t> serializedSeeds;
    Serial::serialize(std::string("_tree"), serializedSeeds);
    Serial::serialize(treeSeeds, serializedSeeds);
    _link->sendBuffer(SerializedObject(ResizableArray(std::move(serializedSeeds))));
}

/*************/
void RootObject::propagatePath(const std::string& path)
{
    assert(_link);
    assert(_link->isReady());

    auto seeds = _tree.getSeedsForPath(path);
    if (seeds.empty())
        return;

    std::vector<uint8_t> serializedSeeds;
    Serial::serialize(std::string("_tree"), serializedSeeds);
    Serial::serialize(seeds, serializedSeeds);
    _link->sendBuffer(SerializedObject(ResizableArray(std::move(serializedSeeds))));
}

/*************/
void RootObject::sendBuffer(SerializedObject&& buffer)
{
    assert(_link);
    assert(_link->isReady());
    _link->sendBuffer(std::move(buffer));
}

/*************/
void RootObject::registerAttributes()
{
    addAttribute("answerMessage",
        [&](const Values& args) {
            if (args.size() == 0 || args[0].as<std::string>() != _answerExpected)
                return false;
            std::unique_lock<std::mutex> conditionLock(_conditionMutex);
            _lastAnswerReceived = args;
            _answerCondition.notify_one();
            return true;
        },
        {});
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
        std::unique_lock<std::recursive_mutex> lock(_attribMutex);
        for (const auto& attribute : _attribFunctions)
        {
            if (!attribute.second.hasGetter())
                continue;
            auto attributeName = attribute.first;
            auto leafPath = path + attributeName;
            if (_tree.hasLeafAt(leafPath))
                continue;
            if (!_tree.createLeafAt(leafPath))
                throw std::runtime_error("Error while adding a leaf at path " + leafPath);

            _treeCallbackIds[attributeName] = _tree.addCallbackToLeafAt(leafPath, [=](const Value& value, const chrono::system_clock::time_point& /*timestamp*/) {
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
Json::Value RootObject::getValuesAsJson(const Values& values, bool asObject) const
{
    Json::Value jsValue;
    if (asObject)
    {
        for (auto& v : values)
        {
            switch (v.getType())
            {
            default:
                continue;
            case Value::boolean:
                jsValue[v.getName()] = v.as<bool>();
                break;
            case Value::integer:
                jsValue[v.getName()] = v.as<int>();
                break;
            case Value::real:
                jsValue[v.getName()] = v.as<float>();
                break;
            case Value::string:
                jsValue[v.getName()] = v.as<std::string>();
                break;
            case Value::values:
            {
                auto vv = v.as<Values>();
                // If the first value is named, we treat it as a Json object
                if (!vv.empty() && vv[0].isNamed())
                    jsValue[v.getName()] = getValuesAsJson(vv, true);
                else
                    jsValue[v.getName()] = getValuesAsJson(vv, false);
                break;
            }
            }
        }
    }
    else
    {
        for (auto& v : values)
        {
            switch (v.getType())
            {
            default:
                continue;
            case Value::boolean:
                jsValue.append(v.as<bool>());
                break;
            case Value::integer:
                jsValue.append(v.as<int>());
                break;
            case Value::real:
                jsValue.append(v.as<float>());
                break;
            case Value::string:
                jsValue.append(v.as<std::string>());
                break;
            case Value::values:
            {
                auto vv = v.as<Values>();
                // If the first value is named, we treat it as a Json object
                if (!vv.empty() && vv[0].isNamed())
                    jsValue.append(getValuesAsJson(vv, true));
                else
                    jsValue.append(getValuesAsJson(vv, false));
                break;
            }
            }
        }
    }
    return jsValue;
}

/*************/
Json::Value RootObject::getObjectConfigurationAsJson(const std::string& object, const std::string& rootObject)
{
    assert(!object.empty());

    std::list<std::string> rootList = {"world"};
    if (!rootObject.empty() && rootObject != "world")
        rootList.push_front(rootObject);
    else if (rootObject.empty() && _name != "world")
        rootList.push_front(_name);

    Json::Value root;
    for (const auto& rootName : rootList)
    {
        const auto attrPath = "/" + rootName + "/objects/" + object + "/attributes";
        const auto docPath = "/" + rootName + "/objects/" + object + "/documentation";
        if (!_tree.hasBranchAt(attrPath))
            continue;

        for (const auto& attrName : _tree.getLeafListAt(attrPath))
        {
            Value attrValue;
            if (!_tree.getValueForLeafAt(attrPath + "/" + attrName, attrValue))
                continue;

            if (attrValue.size() == 0)
                continue;

            // For generated attributes, we only need to save the first value
            // See Attribute documentation/header for more info
            const auto generatedPath = docPath + "/" + attrName + "/generated";
            Value generatedValue;
            const bool generated = _tree.getValueForLeafAt(generatedPath, generatedValue) ? generatedValue.as<bool>() : false;

            Json::Value jsValue;
            if (attrValue.getType() == Value::Type::values && !generated)
                jsValue = getValuesAsJson(attrValue.as<Values>());
            else if (generated)
                jsValue = getValuesAsJson({attrValue[0]});
            else
                jsValue = getValuesAsJson({attrValue});
            root[attrName] = jsValue;
        }

        // Type is handled separately
        const auto typePath = "/" + rootName + "/objects/" + object + "/type";
        if (_tree.hasLeafAt(typePath))
        {
            Value typeValue;
            if (_tree.getValueForLeafAt(typePath, typeValue))
                root["type"] = typeValue.as<std::string>();
        }
    }

    return root;
}

/*************/
Json::Value RootObject::getRootConfigurationAsJson(const std::string& rootName)
{
    if (!_tree.hasBranchAt("/" + rootName))
        return {};

    Json::Value root;
    const auto attrPath = "/" + rootName + "/attributes";
    assert(_tree.hasBranchAt(attrPath));

    for (const auto& attrName : _tree.getLeafListAt(attrPath))
    {
        Value attrValue;
        if (!_tree.getValueForLeafAt(attrPath + "/" + attrName, attrValue))
            continue;

        if (attrValue.size() == 0)
            continue;

        Json::Value jsValue;
        if (attrValue.getType() == Value::Type::values)
            jsValue = getValuesAsJson(attrValue.as<Values>());
        else
            jsValue = getValuesAsJson({attrValue});

        root[attrName] = jsValue;
    }

    // The world's objects are created as surrogates for the scene's ones
    if (rootName == "world")
        return root;

    // Save objects attributes and links
    root["objects"] = Json::Value();
    root["links"] = Json::Value();

    const auto objectsPath = "/" + rootName + "/objects";
    assert(_tree.hasBranchAt(objectsPath));
    for (const auto& objectName : _tree.getBranchListAt(objectsPath))
    {
        Value confValue;
        if (_tree.getValueForLeafAt(objectsPath + "/" + objectName + "/attributes/savable", confValue) && !confValue.isEmpty() && confValue[0].as<bool>() == false)
            continue;
        if (_tree.getValueForLeafAt(objectsPath + "/" + objectName + "/ghost", confValue) && confValue.as<bool>() == true)
            continue;
        root["objects"][objectName] = getObjectConfigurationAsJson(objectName, rootName);

        assert(_tree.hasLeafAt(objectsPath + "/" + objectName + "/links/children"));
        Value value;
        _tree.getValueForLeafAt(objectsPath + "/" + objectName + "/links/children", value);
        for (const auto& parent : value.as<Values>())
        {
            const auto linkName = parent.as<std::string>();
            if (_tree.getValueForLeafAt(objectsPath + "/" + linkName + "/attributes/savable", confValue) && !confValue.isEmpty() && confValue[0].as<bool>() == false)
                continue;
            if (_tree.getValueForLeafAt(objectsPath + "/" + linkName + "/ghost", confValue) && confValue.as<bool>() == true)
                continue;
            root["links"].append(getValuesAsJson({linkName, objectName}));
        }
    }

    return root;
}

/*************/
Values RootObject::sendMessageWithAnswer(const std::string& name, const std::string& attribute, const Values& message, const unsigned long long timeout)
{
    assert(_link);
    assert(_link->isReady());

    std::lock_guard<std::mutex> lock(_answerMutex);
    _answerExpected = attribute;

    std::unique_lock<std::mutex> conditionLock(_conditionMutex);
    _link->sendMessage(name, attribute, message);

    auto cvStatus = std::cv_status::no_timeout;
    if (timeout == 0ull)
        _answerCondition.wait(conditionLock);
    else
        cvStatus = _answerCondition.wait_for(conditionLock, chrono::microseconds(timeout));

    _answerExpected = "";

    if (std::cv_status::no_timeout == cvStatus)
        return _lastAnswerReceived;
    else
        return {};
}

} // namespace Splash
