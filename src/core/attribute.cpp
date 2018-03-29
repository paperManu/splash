#include "./core/attribute.h"

#include "./core/base_object.h"
#include "./utils/log.h"

using namespace std;

namespace Splash
{

atomic_uint CallbackHandle::_nextCallbackId{1};

/*************/
CallbackHandle::~CallbackHandle()
{
    auto owner = _owner.lock();
    if (owner)
        owner->unregisterCallback(*this);
}

/*************/
AttributeFunctor::AttributeFunctor(const string& name, const function<bool(const Values&)>& setFunc, const vector<char>& types)
    : _name(name)
    , _setFunc(setFunc)
    , _getFunc(function<const Values()>())
    , _defaultSetAndGet(false)
    , _valuesTypes(types)
{
}

/*************/
AttributeFunctor::AttributeFunctor(const string& name, const function<bool(const Values&)>& setFunc, const function<const Values()>& getFunc, const vector<char>& types)
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
        _defaultSetAndGet = a._defaultSetAndGet;
        _doUpdateDistant = a._doUpdateDistant;
        _savable = a._savable;
    }

    return *this;
}

/*************/
bool AttributeFunctor::operator()(const Values& args)
{
    if (_isLocked)
        return false;

    // Run all set callbacks
    {
        lock_guard<mutex> lockCb(_callbackMutex);
        for (auto& cb : _callbacks)
            cb.second(_objectName, _name);
    }

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

    for (uint32_t i = 0; i < _valuesTypes.size(); ++i)
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
bool AttributeFunctor::lock(const Values& v)
{
    if (v.size() != 0)
        if (!operator()(v))
            return false;

    _isLocked = true;
    return true;
}

/*************/
CallbackHandle AttributeFunctor::registerCallback(weak_ptr<BaseObject> caller, Callback cb)
{
    lock_guard<mutex> lockCb(_callbackMutex);
    auto handle = CallbackHandle(caller, _name);
    _callbacks[handle.getId()] = cb;
    return handle;
}

/*************/
bool AttributeFunctor::unregisterCallback(const CallbackHandle& handle)
{
    lock_guard<mutex> lockCb(_callbackMutex);
    auto callback = _callbacks.find(handle.getId());
    if (callback == _callbacks.end())
        return false;

    _callbacks.erase(callback);
    return true;
}

} // end of namespace
