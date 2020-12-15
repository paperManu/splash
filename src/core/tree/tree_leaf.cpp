#include "./core/tree/tree_leaf.h"

#include "./core/tree/tree_branch.h"
#include "./utils/log.h"

namespace chrono = std::chrono;

namespace Splash
{

namespace Tree
{

/*************/
Leaf::Leaf(const std::string& name, Value value, Branch* branch)
    : _name(name)
    , _value(value)
    , _parentBranch(branch)
{
}

/*************/
bool Leaf::operator==(const Leaf& rhs) const
{
    return (_value == rhs._value);
}

/*************/
Leaf::UpdateCallbackID Leaf::addCallback(const UpdateCallback& callback)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    auto id = ++_currentCallbackID;
    _callbacks[id] = callback;
    return id;
}

/*************/
bool Leaf::removeCallback(int id)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    auto callbackIt = _callbacks.find(id);
    if (callbackIt == _callbacks.end())
        return false;
    _callbacks.erase(callbackIt);
    return true;
}

/*************/
std::string Leaf::getPath() const
{
    std::string path{};
    if (_parentBranch)
        path = _parentBranch->getPath() + _name + "/";
    else
        path = _name;

    return path;
}

/*************/
std::string Leaf::print(int indent) const
{
    std::string description;
    for (int i = 0; i < indent / 2; ++i)
        description += "| ";
    description += "|-- " + _name + "\n";

    if (_value.getType() == Value::Type::values)
    {
        for (const auto& v : _value.as<Values>())
        {
            for (int i = 0; i < (indent + 2) / 2; ++i)
                description += "| ";
            description += "|-- " + v.as<std::string>() + "\n";
        }
    }
    else
    {
        for (int i = 0; i < (indent + 2) / 2; ++i)
            description += "| ";
        description += "|-- " + _value.as<std::string>() + "\n";
    }

    return description;
}

/*************/
bool Leaf::set(Value value, chrono::system_clock::time_point timestamp)
{
    if (timestamp < _timestamp)
        return false;

    _timestamp = timestamp;
    _value = value;

    std::lock_guard<std::mutex> lock(_callbackMutex);
    for (const auto& callback : _callbacks)
        callback.second(value, _timestamp);

    return true;
}

} // namespace Tree

} // namespace Splash
