#include "./userinput_joystick.h"

#include <regex>

using namespace std;

namespace Splash
{
/*************/
Joystick::Joystick(RootObject* root)
    : UserInput(root)
{
    _type = "joystick";
}

/*************/
Joystick::~Joystick()
{
}

/*************/
void Joystick::detectJoysticks()
{
    int nbrJoysticks = 0;
    for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; ++i)
        if (glfwJoystickPresent(i))
            nbrJoysticks = i + 1 - GLFW_JOYSTICK_1;

    _joysticks.resize(nbrJoysticks);
}

/*************/
void Joystick::updateMethod()
{
    detectJoysticks();

    for (uint32_t i = 0; i < _joysticks.size(); ++i)
    {
        auto& joystick = _joysticks[i];
        int count;
        auto bufferAxes = glfwGetJoystickAxes(GLFW_JOYSTICK_1 + i, &count);
        auto axes = vector<float>(bufferAxes, bufferAxes + count);

        // TODO: axes configuration, in this case for the dead zone
        for (auto& a : axes)
            if (abs(a) < 0.2f)
                a = 0.f;

        if (joystick.axes.size() < axes.size())
            for (uint32_t a = 0; a < axes.size() - joystick.axes.size(); ++a)
                joystick.axes.push_back(0.f);

        // Axes values are accumulated until they are read
        for (uint32_t a = 0; a < joystick.axes.size(); ++a)
            joystick.axes[a] += axes[a];

        auto bufferButtons = glfwGetJoystickButtons(GLFW_JOYSTICK_1 + i, &count);
        joystick.buttons = vector<uint8_t>(bufferButtons, bufferButtons + count);
    }
}

/*************/
void Joystick::updateCallbacks()
{
    for (auto& callbackIt : _callbacks)
    {
        const auto& targetState = callbackIt.first;
        const auto& callback = callbackIt.second;

        int joystickIndex = -1;
        string joystickAction = "";

        regex regexAction("joystick_([0-9]+)_(.+)");
        smatch match;
        try
        {
            if (regex_match(targetState.action, match, regexAction) && match.size() == 3)
            {
                joystickIndex = stoi(match[1].str());
                joystickAction = match[2].str();
            }
        }
        catch (...)
        {
            Log::get() << Log::WARNING << "Joystick::" << __FUNCTION__ << " - Error while matching regex" << Log::endl;
            return;
        }

        if (joystickIndex != -1 && joystickAction.size() != 0 && static_cast<int>(_joysticks.size()) > joystickIndex)
        {
            State state(targetState);
            if (joystickAction == "buttons")
            {
                for (auto& b : _joysticks[joystickIndex].buttons)
                    state.value.push_back(b);
            }
            else if (joystickAction == "axes")
            {
                for (auto& a : _joysticks[joystickIndex].axes)
                {
                    state.value.push_back(a);
                    a = 0.f; // Reset joystick axis accumulation
                }
            }
            else
            {
                return;
            }

            callback(state);
        }
    }
}

/*************/
void Joystick::readState()
{
    int index = 0;
    for (auto& joystick : _joysticks)
    {
        State joystate;
        joystate.action = "joystick_" + to_string(index) + "_axes";
        for (auto& a : joystick.axes)
        {
            joystate.value.push_back(a);
            a = 0.f; // Reset joystick axis accumulation
        }
        _state.push_back(joystate);

        joystate.action = "joystick_" + to_string(index) + "_buttons";
        joystate.value.clear();
        for (auto& b : joystick.buttons)
            joystate.value.push_back(b);
        _state.push_back(joystate);

        ++index;
    }
}

} // end of namespace
