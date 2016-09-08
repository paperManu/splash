#include "./userInput_joystick.h"

using namespace std;

namespace Splash
{
/*************/
Joystick::Joystick(weak_ptr<RootObject> root) : UserInput(root)
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

    for (int i = 0; i < _joysticks.size(); ++i)
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
            for (int a = 0; a < axes.size() - joystick.axes.size(); ++a)
                joystick.axes.push_back(0.f);

        // Axes values are accumulated until they are read
        for (int a = 0; a < joystick.axes.size(); ++a)
            joystick.axes[a] += axes[a];

        auto bufferButtons = glfwGetJoystickButtons(GLFW_JOYSTICK_1 + i, &count);
        joystick.buttons = vector<uint8_t>(bufferButtons, bufferButtons + count);
    }
}

/*************/
void Joystick::readState()
{
    _state.clear();

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
