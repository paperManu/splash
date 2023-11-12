#include "./core/attribute.h"

#include "./core/graph_object.h"
#include "./utils/log.h"

namespace Splash
{

std::atomic_uint CallbackHandle::_nextCallbackId{1};

/*************/
CallbackHandle::~CallbackHandle()
{
    auto owner = _owner.lock();
    if (owner)
        owner->unregisterCallback(*this);
}

/*************/
Attribute::Attribute(const std::string& name, const std::function<bool(const Values&)>& setFunc, const std::function<Values()>& getFunc, const std::vector<char>& types)
    : _name(name)
    , _valuesTypes(types)
    , _setFunc(setFunc)
    , _getFunc(getFunc)
{
}

/*************/
Attribute::Attribute(const std::string& name, const std::function<bool(const Values&)>& setFunc, const std::vector<char>& types)
    : _name(name)
    , _valuesTypes(types)
    , _setFunc(setFunc)
    , _getFunc(nullptr)
{
}

/*************/
Attribute::Attribute(const std::string& name, const std::function<Values()>& getFunc)
    : _name(name)
    , _setFunc(nullptr)
    , _getFunc(getFunc)
{
}

/*************/
Attribute& Attribute::operator=(Attribute&& a) noexcept
{
    if (this != &a)
    {
        _name = std::move(a._name);
        _objectName = std::move(a._objectName);
        _description = std::move(a._description);
        _valuesTypes = std::move(a._valuesTypes);

        _setFunc = std::move(a._setFunc);
        _getFunc = std::move(a._getFunc);

        _syncMethod = a._syncMethod;
        _callbacks = std::move(a._callbacks);
        _isLocked = a._isLocked;
    }

    return *this;
}

/*************/
bool Attribute::operator()(const Values& args)
{
    if (_isLocked)
        return false;

    if (!_setFunc)
        return false;

    // Check for arguments correctness.
    // Some attributes may have an unlimited number of arguments, so we do not test for equality.
    if (args.size() < _valuesTypes.size())
    {
        Log::get() << Log::WARNING << _objectName << "~~" << _name << " - Wrong number of arguments (" << args.size() << " instead of " << _valuesTypes.size() << ")" << Log::endl;
        return false;
    }

    for (uint32_t i = 0; i < _valuesTypes.size(); ++i)
    {
        if (args[i].isConvertibleToType(Value::getTypeFromChar(_valuesTypes[i])))
            continue;

        const auto type = args[i].getTypeAsChar();
        const auto expected = _valuesTypes[i];

        Log::get() << Log::WARNING << _objectName << "~~" << _name << " - Argument " << i << " is of wrong type " << std::string(&type, &type + 1) << ", expected "
                   << std::string(&expected, &expected + 1) << Log::endl;
        return false;
    }

    const auto returnValue = _setFunc(args);

    // Run all set callbacks
    if (!_callbacks.empty())
    {
        std::lock_guard<std::mutex> lockCb(_callbackMutex);
        for (const auto& cb : _callbacks)
            cb.second(_objectName, _name);
    }

    return returnValue;
}

/*************/
Values Attribute::operator()() const
{
    if (!_getFunc)
        return Values();

    return _getFunc();
}

/*************/
Values Attribute::getArgsTypes() const
{
    Values types{};
    for (const auto& type : _valuesTypes)
        types.push_back(Value(std::string(&type, 1)));
    return types;
}

/*************/
bool Attribute::lock(const Values& v)
{
    if (v.size() != 0)
        if (!operator()(v))
            return false;

    _isLocked = true;
    return true;
}

/*************/
CallbackHandle Attribute::registerCallback(std::weak_ptr<BaseObject> caller, Callback cb)
{
    std::lock_guard<std::mutex> lockCb(_callbackMutex);
    auto handle = CallbackHandle(caller, _name);
    _callbacks[handle.getId()] = cb;
    return handle;
}

/*************/
bool Attribute::unregisterCallback(const CallbackHandle& handle)
{
    std::lock_guard<std::mutex> lockCb(_callbackMutex);
    const auto callback = _callbacks.find(handle.getId());
    if (callback == _callbacks.end())
        return false;

    _callbacks.erase(callback);
    return true;
}

} // namespace Splash
