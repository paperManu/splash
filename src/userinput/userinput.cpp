#include "./userinput/userinput.h"

#include "./core/scene.h"

namespace chrono = std::chrono;

namespace Splash
{

std::mutex UserInput::_callbackMutex;
std::map<UserInput::State, std::function<void(const UserInput::State&)>, UserInput::StateCompare> UserInput::_callbacks;

/*************/
UserInput::State::State(const std::string& a, const Values& v, int m, const std::string& w)
    : modifiers(m)
    , action(a)
    , window(w)
    , value(v)
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
    _updateThread = std::thread([&]() {
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
bool UserInput::capture(const std::string& id)
{
    std::lock_guard<std::mutex> lock(_captureMutex);
    if (_captured)
        return false;

    _captured = true;
    _capturer = id;
    return true;
}

/*************/
std::vector<UserInput::State> UserInput::getState(const std::string& id)
{
    std::lock_guard<std::mutex> lockState(_stateMutex);
    std::lock_guard<std::mutex> lockCapture(_captureMutex);

    if (_captured && _capturer != id)
        return {};

    readState();

    auto state(_state);
    _state.clear();
    return state;
}

/*************/
void UserInput::release(const std::string& id)
{
    std::lock_guard<std::mutex> lock(_captureMutex);
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
    std::lock_guard<std::mutex> lock(_callbackMutex);
    const auto callbackIt = _callbacks.find(state);
    if (callbackIt != _callbacks.end())
        _callbacks.erase(callbackIt);
}

/*************/
void UserInput::setCallback(const UserInput::State& state, const std::function<void(const UserInput::State&)>& cb)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    _callbacks[state] = cb;
}

/*************/
std::string UserInput::getWindowName(const GLFWwindow* glfwWindow) const
{
    if (!glfwWindow)
        return {};

    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return {};

    auto windows = std::list<std::shared_ptr<GraphObject>>();
    auto lock = scene->getLockOnObjects();
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == "window")
            windows.push_back(obj.second);

    for (auto& w : windows)
    {
        auto window = std::dynamic_pointer_cast<Window>(w);
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
            std::lock_guard<std::mutex> lockStates(_stateMutex);
            updateMethod();

            std::lock_guard<std::mutex> lockCallbacks(_callbackMutex);
            updateCallbacks();
        }
        auto end = Timer::getTime();
        auto delta = end - start;
        int64_t loopDuration = 1e6 / _updateRate;
        std::this_thread::sleep_for(chrono::microseconds(loopDuration - delta));
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
            _updateRate = std::max(10, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_updateRate}; },
        {'i'});
    setAttributeDescription("updateRate", "Set the rate at which the inputs are updated");
}

} // end of namespace
