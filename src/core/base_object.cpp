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
} // namespace Splash
