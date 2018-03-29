#include "./core/base_object.h"

#include <algorithm>

#include "./utils/log.h"

using namespace std;

namespace Splash
{

/*************/
AttributeFunctor& BaseObject::operator[](const string& attr)
{
    auto attribFunction = _attribFunctions.find(attr);
    return attribFunction->second;
}

/*************/
void BaseObject::addTask(const function<void()>& task)
{
    lock_guard<recursive_mutex> lock(_taskMutex);
    _taskQueue.push_back(task);
}

/*************/
void BaseObject::linkToParent(BaseObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt == _parents.end())
        _parents.push_back(obj);
    return;
}

/*************/
void BaseObject::unlinkFromParent(BaseObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt != _parents.end())
        _parents.erase(parentIt);
    return;
}

/*************/
bool BaseObject::linkTo(const shared_ptr<BaseObject>& obj)
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
        obj->linkToParent(this);
        return true;
    }
    return false;
}

/*************/
void BaseObject::unlinkFrom(const shared_ptr<BaseObject>& obj)
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
    {
        _linkedObjects.erase(objectIt);
        obj->unlinkFromParent(this);
    }
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
        auto result = _attribFunctions.emplace(attrib, AttributeFunctor(attrib));
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
    {
        args.clear();
        return false;
    }

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
Json::Value BaseObject::getValuesAsJson(const Values& values, bool asObject) const
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
            case Value::i:
                jsValue[v.getName()] = v.as<int>();
                break;
            case Value::f:
                jsValue[v.getName()] = v.as<float>();
                break;
            case Value::s:
                jsValue[v.getName()] = v.as<string>();
                break;
            case Value::v:
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
CallbackHandle BaseObject::registerCallback(const string& attr, AttributeFunctor::Callback cb)
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
bool BaseObject::setRenderingPriority(Priority priority)
{
    if (priority < Priority::PRE_CAMERA || priority >= Priority::POST_WINDOW)
        return false;
    _renderingPriority = priority;
    return true;
}

/*************/
void BaseObject::runAsyncTask(const function<void(void)>& func)
{
    lock_guard<mutex> lockTasks(_asyncTaskMutex);
    _asyncTask = async(launch::async, func);
}

/*************/
void BaseObject::registerAttributes()
{
    addAttribute("setName",
        [&](const Values& args) {
            setName(args[0].as<string>());
            return true;
        },
        {'s'});

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

/*************/
AttributeFunctor& BaseObject::addAttribute(const string& name, const function<bool(const Values&)>& set, const vector<char>& types)
{
    _attribFunctions[name] = AttributeFunctor(name, set, types);
    _attribFunctions[name].setObjectName(_type);
    return _attribFunctions[name];
}

/*************/
AttributeFunctor& BaseObject::addAttribute(const string& name, const function<bool(const Values&)>& set, const function<const Values()>& get, const vector<char>& types)
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
void BaseObject::runTasks()
{
    lock_guard<recursive_mutex> lock(_taskMutex);
    for (auto& task : _taskQueue)
        task();
    _taskQueue.clear();
}
}
