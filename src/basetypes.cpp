#include "./basetypes.h"

using namespace std;

namespace Splash
{
/*************/
// AttributeFunctor
/*************/
AttributeFunctor::AttributeFunctor(const string& name, function<bool(const Values&)> setFunc, const vector<char>& types)
    : _name(name)
    , _setFunc(setFunc)
    , _getFunc(function<const Values()>())
    , _defaultSetAndGet(false)
    , _valuesTypes(types)
{
}

/*************/
AttributeFunctor::AttributeFunctor(const string& name, function<bool(const Values&)> setFunc, function<const Values()> getFunc, const vector<char>& types)
    : _name(name)
    , _setFunc(setFunc)
    , _getFunc(getFunc)
    , _defaultSetAndGet(false)
    , _valuesTypes(types)
{
}

/*************/
AttributeFunctor& AttributeFunctor::operator=(AttributeFunctor&& a)
{
    if (this != &a)
    {
        _name = move(a._name);
        _objectName = move(a._objectName);
        _setFunc = move(a._setFunc);
        _getFunc = move(a._getFunc);
        _description = move(a._description);
        _values = move(a._values);
        _valuesTypes = move(a._valuesTypes);
        _defaultSetAndGet = move(a._defaultSetAndGet);
        _doUpdateDistant = move(a._doUpdateDistant);
        _savable = move(a._savable);
    }

    return *this;
}

/*************/
bool AttributeFunctor::operator()(const Values& args)
{
    if (_isLocked)
        return false;

    if (!_setFunc && _defaultSetAndGet)
    {
        lock_guard<mutex> lock(_defaultFuncMutex);
        _values = args;

        _valuesTypes.clear();
        for (const auto& a : args)
            _valuesTypes.push_back(a.getTypeAsChar());

        return true;
    }
    else if (!_setFunc)
    {
        return false;
    }

    // Check for arguments correctness.
    // Some attributes may have an unlimited number of arguments, so we do not test for equality.
    if (args.size() < _valuesTypes.size())
    {
        Log::get() << Log::WARNING << _objectName << "~~" << _name << " - Wrong number of arguments (" << args.size() << " instead of " << _valuesTypes.size() << ")" << Log::endl;
        return false;
    }

    for (int i = 0; i < _valuesTypes.size(); ++i)
    {
        auto type = args[i].getTypeAsChar();
        auto expected = _valuesTypes[i];

        if (type != expected)
        {
            Log::get() << Log::WARNING << _objectName << "~~" << _name << " - Argument " << i << " is of wrong type " << string(&type, &type + 1) << ", expected "
                       << string(&expected, &expected + 1) << Log::endl;
            return false;
        }
    }

    return _setFunc(forward<const Values&>(args));
}

/*************/
Values AttributeFunctor::operator()() const
{
    if (!_getFunc && _defaultSetAndGet)
    {
        lock_guard<mutex> lock(_defaultFuncMutex);
        return _values;
    }
    else if (!_getFunc)
    {
        return Values();
    }

    return _getFunc();
}

/*************/
Values AttributeFunctor::getArgsTypes() const
{
    Values types{};
    for (const auto& type : _valuesTypes)
        types.push_back(Value(string(&type, 1)));
    return types;
}

/*************/
bool AttributeFunctor::lock(Values v)
{
    if (v.size() != 0)
        if (!operator()(v))
            return false;

    _isLocked = true;
    return true;
}

/*************/
// BaseObject
/*************/
AttributeFunctor& BaseObject::operator[](const string& attr)
{
    auto attribFunction = _attribFunctions.find(attr);
    return attribFunction->second;
}

/*************/
bool BaseObject::linkTo(shared_ptr<BaseObject> obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const weak_ptr<BaseObject>& o) {
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
        return true;
    }
    return false;
}

/*************/
void BaseObject::unlinkFrom(shared_ptr<BaseObject> obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const weak_ptr<BaseObject>& o) {
        auto object = o.lock();
        if (!object)
            return false;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt != _linkedObjects.end())
        _linkedObjects.erase(objectIt);
}

/*************/
const vector<shared_ptr<BaseObject>> BaseObject::getLinkedObjects()
{
    vector<shared_ptr<BaseObject>> objects;
    for (auto& o : _linkedObjects)
    {
        auto obj = o.lock();
        if (!obj)
            continue;

        objects.push_back(obj);
    }

    return objects;
}

/*************/
bool BaseObject::setAttribute(const string& attrib, const Values& args)
{
    auto attribFunction = _attribFunctions.find(attrib);
    bool attribNotPresent = (attribFunction == _attribFunctions.end());

    if (attribNotPresent)
    {
        auto result = _attribFunctions.emplace(attrib, AttributeFunctor());
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
bool BaseObject::getAttribute(const string& attrib, Values& args, bool includeDistant, bool includeNonSavable) const
{
    auto attribFunction = _attribFunctions.find(attrib);
    if (attribFunction == _attribFunctions.end())
        return false;

    args = attribFunction->second();

    if ((!attribFunction->second.savable() && !includeNonSavable) || (attribFunction->second.isDefault() && !includeDistant))
        return false;

    return true;
}

/*************/
unordered_map<string, Values> BaseObject::getAttributes(bool includeDistant) const
{
    unordered_map<string, Values> attribs;
    for (auto& attr : _attribFunctions)
    {
        Values values;
        if (getAttribute(attr.first, values, includeDistant, true) == false || values.size() == 0)
            continue;
        attribs[attr.first] = values;
    }

    return attribs;
}

/*************/
unordered_map<string, Values> BaseObject::getDistantAttributes() const
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
Json::Value BaseObject::getValuesAsJson(const Values& values) const
{
    Json::Value jsValue;
    for (auto& v : values)
    {
        switch (v.getType())
        {
        default:
            continue;
        case Value::i:
            jsValue.append(v.as<int>());
            break;
        case Value::f:
            jsValue.append(v.as<float>());
            break;
        case Value::s:
            jsValue.append(v.as<string>());
            break;
        case Value::v:
            jsValue.append(getValuesAsJson(v.as<Values>()));
            break;
        }
    }
    return jsValue;
}

/*************/
Json::Value BaseObject::getConfigurationAsJson() const
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
AttributeFunctor::Sync BaseObject::getAttributeSyncMethod(const string& name)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
        return attr->second.getSyncMethod();
    else
        return AttributeFunctor::Sync::no_sync;
}

/*************/
bool BaseObject::setRenderingPriority(Priority priority)
{
    if (priority < Priority::PRE_CAMERA || priority >= Priority::POST_WINDOW)
        return false;
    _renderingPriority = priority;
    return true;
}

/*************/
void BaseObject::init()
{
    addAttribute("setName",
        [&](const Values& args) {
            setName(args[0].as<string>());
            return true;
        },
        {'s'});

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

/*************/
AttributeFunctor& BaseObject::addAttribute(const string& name, function<bool(const Values&)> set, const vector<char> types)
{
    _attribFunctions[name] = AttributeFunctor(name, set, types);
    _attribFunctions[name].setObjectName(_type);
    return _attribFunctions[name];
}

/*************/
AttributeFunctor& BaseObject::addAttribute(const string& name, function<bool(const Values&)> set, function<const Values()> get, const vector<char>& types)
{
    _attribFunctions[name] = AttributeFunctor(name, set, get, types);
    _attribFunctions[name].setObjectName(_type);
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
void BaseObject::setAttributeSyncMethod(const string& name, const AttributeFunctor::Sync& method)
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
void BaseObject::setAttributeParameter(const string& name, bool savable, bool updateDistant)
{
    auto attr = _attribFunctions.find(name);
    if (attr != _attribFunctions.end())
    {
        attr->second.savable(savable);
        attr->second.doUpdateDistant(updateDistant);
    }
}

/*************/
// BufferObject
/*************/
void BufferObject::setNotUpdated()
{
    BaseObject::setNotUpdated();
    _updatedBuffer = false;
}

/*************/
bool BufferObject::deserialize()
{
    if (!_newSerializedObject)
        return false;

    bool returnValue = deserialize(_serializedObject);
    _newSerializedObject = false;

    return returnValue;
}

/*************/
void BufferObject::setSerializedObject(shared_ptr<SerializedObject> obj)
{
    bool expectedAtomicValue = false;
    if (_serializedObjectWaiting.compare_exchange_strong(expectedAtomicValue, true))
    {
        _serializedObject = move(obj);
        _newSerializedObject = true;

        // Deserialize it right away, in a separate thread
        SThread::pool.enqueueWithoutId([this]() {
            lock_guard<Spinlock> lock(_writeMutex);
            deserialize();
            _serializedObjectWaiting = false;
        });
    }
}

/*************/
void BufferObject::updateTimestamp()
{
    _timestamp = Timer::getTime();
    _updatedBuffer = true;
    _root.lock()->signalBufferObjectUpdated();
}

/*************/
// RootObject
/*************/
RootObject::RootObject()
{
    registerAttributes();
}

/*************/
void RootObject::registerObject(shared_ptr<BaseObject> object)
{
    if (object.get() != nullptr)
    {
        auto name = object->getName();

        lock_guard<recursive_mutex> registerLock(_objectsMutex);
        object->setSavable(false); // This object was created on the fly. Do not save it

        // We keep the previous object on the side, to prevent double free due to operator[] behavior
        auto previousObject = shared_ptr<BaseObject>();
        auto objectIt = _objects.find(name);
        if (objectIt != _objects.end())
            previousObject = objectIt->second;

        _objects[name] = object;
    }
}

/*************/
shared_ptr<BaseObject> RootObject::unregisterObject(const string& name)
{
    lock_guard<recursive_mutex> lock(_objectsMutex);

    auto objectIt = _objects.find(name);
    if (objectIt != _objects.end())
    {
        auto object = objectIt->second;
        _objects.erase(objectIt);
        return object;
    }

    return {};
}

/*************/
bool RootObject::set(const string& name, const string& attrib, const Values& args, bool async)
{
    lock_guard<recursive_mutex> lock(_setMutex);

    if (name == _name || name == SPLASH_ALL_PEERS)
        return setAttribute(attrib, args);

    auto objectIt = _objects.find(name);
    if (objectIt != _objects.end() && objectIt->second->getAttributeSyncMethod(attrib) == AttributeFunctor::Sync::force_sync)
        async = false;

    if (async)
    {
        addTask([=]() {
            auto objectIt = _objects.find(name);
            if (objectIt != _objects.end())
                objectIt->second->setAttribute(attrib, args);
        });
    }
    else
    {
        if (objectIt != _objects.end())
            return objectIt->second->setAttribute(attrib, args);
        else
            return false;
    }

    return true;
}

/*************/
void RootObject::setFromSerializedObject(const string& name, shared_ptr<SerializedObject> obj)
{
    lock_guard<recursive_mutex> lock(_setMutex);

    auto objectIt = _objects.find(name);
    if (objectIt != _objects.end())
    {
        auto object = dynamic_pointer_cast<BufferObject>(objectIt->second);
        if (object)
            object->setSerializedObject(move(obj));
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
    _bufferObjectUpdatedCondition.notify_all();
}

/*************/
bool RootObject::waitSignalBufferObjectUpdated(uint64_t timeout)
{
    unique_lock<mutex> lockCondition(_bufferObjectUpdatedMutex);
    auto status = _bufferObjectUpdatedCondition.wait_for(lockCondition, chrono::microseconds(timeout));
    return (status == cv_status::no_timeout);
}

/*************/
void RootObject::addTask(const function<void()>& task)
{
    lock_guard<recursive_mutex> lock(_taskMutex);
    _taskQueue.push_back(task);
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
void RootObject::runTasks()
{
    lock_guard<recursive_mutex> lock(_taskMutex);
    for (auto& task : _taskQueue)
        task();
    _taskQueue.clear();
}

/*************/
Values RootObject::sendMessageWithAnswer(string name, string attribute, const Values& message, const unsigned long long timeout)
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

} // end of namespace
