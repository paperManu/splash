#include "./userinput/userinput.h"

#include "./core/scene.h"

using namespace std;

namespace Splash
{

mutex UserInput::_callbackMutex;
map<UserInput::State, function<void(const UserInput::State&)>, UserInput::StateCompare> UserInput::_callbacks;

/*************/
UserInput::State::State(const string& a, const Values& v, int m, const string& w)
    : action(a)
    , value(v)
    , modifiers(m)
    , window(w)
{
}

/*************/
bool UserInput::State::operator==(const UserInput::State& s) const
{
    if (s.action == action && s.modifiers == modifiers)
        return true;
    else
        return false;
}

/*************/
UserInput::UserInput(RootObject* root)
    : GraphObject(root)
{
    _type = "userinput";
    registerAttributes();
    _updateThread = thread([&]() {
        _running = true;
        updateLoop();
    });
}

/*************/
UserInput::~UserInput()
{
    _running = false;
    if (_updateThread.joinable())
        _updateThread.join();
}

/*************/
bool UserInput::capture(const string& id)
{
    lock_guard<mutex> lock(_captureMutex);
    if (_captured)
        return false;

    _captured = true;
    _capturer = id;
    return true;
}

/*************/
vector<UserInput::State> UserInput::getState(const string& id)
{
    lock_guard<mutex> lockState(_stateMutex);
    lock_guard<mutex> lockCapture(_captureMutex);

    if (_captured && _capturer != id)
        return {};

    readState();

    auto state(_state);
    _state.clear();
    return state;
}

/*************/
void UserInput::release(const string& id)
{
    lock_guard<mutex> lock(_captureMutex);
    if (id == _capturer)
    {
        _captured = false;
        _capturer = "";
    }
}

/*************/
bool UserInput::StateCompare::operator()(const UserInput::State& lhs, const UserInput::State& rhs) const
{
    if (lhs.action < rhs.action)
    {
        return true;
    }
    else if (rhs.action == lhs.action)
    {
        if (lhs.modifiers < rhs.modifiers)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}

/*************/
void UserInput::resetCallback(const UserInput::State& state)
{
    lock_guard<mutex> lock(_callbackMutex);
    const auto callbackIt = _callbacks.find(state);
    if (callbackIt != _callbacks.end())
        _callbacks.erase(callbackIt);
}

/*************/
void UserInput::setCallback(const UserInput::State& state, const function<void(const UserInput::State&)>& cb)
{
    lock_guard<mutex> lock(_callbackMutex);
    _callbacks[state] = cb;
}

/*************/
string UserInput::getWindowName(const GLFWwindow* glfwWindow) const
{
    if (!glfwWindow)
        return {};

    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return {};

    auto windows = list<shared_ptr<GraphObject>>();
    auto lock = scene->getLockOnObjects();
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == "window")
            windows.push_back(obj.second);

    for (auto& w : windows)
    {
        auto window = dynamic_pointer_cast<Window>(w);
        if (window->isWindow(const_cast<GLFWwindow*>(glfwWindow)))
            return window->getName();
    }

    return {};
}

/*************/
void UserInput::updateLoop()
{
    while (_running)
    {
        auto start = Timer::getTime();
        {
            lock_guard<mutex> lockStates(_stateMutex);
            updateMethod();

            lock_guard<mutex> lockCallbacks(_callbackMutex);
            updateCallbacks();
        }
        auto end = Timer::getTime();
        auto delta = end - start;
        int64_t loopDuration = 1e6 / _updateRate;
        this_thread::sleep_for(chrono::microseconds(loopDuration - delta));
    }
}

/*************/
void UserInput::updateCallbacks()
{
    for (auto stateIt = _state.begin(); stateIt != _state.end();)
    {
        const auto callbackIt = _callbacks.find(*stateIt);
        if (callbackIt != _callbacks.cend())
        {
            callbackIt->second(*stateIt);
            stateIt = _state.erase(stateIt);
        }
        else
        {
            ++stateIt;
        }
    }
}

/*************/
void UserInput::registerAttributes()
{
    GraphObject::registerAttributes();

    addAttribute("updateRate",
        [&](const Values& args) {
            _updateRate = max(10, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_updateRate}; },
        {'n'});
    setAttributeDescription("updateRate", "Set the rate at which the inputs are updated");
}

} // end of namespace
