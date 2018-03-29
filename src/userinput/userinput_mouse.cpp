#include "./userinput/userinput_mouse.h"

using namespace std;

namespace Splash
{
/*************/
Mouse::Mouse(RootObject* root)
    : UserInput(root)
{
    _type = "mouse";
}

/*************/
Mouse::~Mouse()
{
}

/*************/
void Mouse::updateMethod()
{
    // Mouse position
    {
        GLFWwindow* win;
        int xpos, ypos;
        Window::getMousePos(win, xpos, ypos);

        State substate;
        substate.action = "mouse_position";
        substate.value = Values({xpos, ypos});
        substate.window = getWindowName(win);
        _state.emplace_back(std::move(substate));
    }

    // Buttons events
    while (true)
    {
        GLFWwindow* win;
        int btn, action, mods;
        if (!Window::getMouseBtn(win, btn, action, mods))
            break;

        State substate;
        switch (action)
        {
        default:
            continue;
        case GLFW_PRESS:
            substate.action = "mouse_press";
            break;
        case GLFW_RELEASE:
            substate.action = "mouse_release";
            break;
        }
        substate.value = Values({Value(btn)});
        substate.modifiers = mods;
        substate.window = getWindowName(win);
        _state.emplace_back(std::move(substate));
    }

    // Scrolling events
    while (true)
    {
        GLFWwindow* win;
        double xoffset, yoffset;
        if (!Window::getScroll(win, xoffset, yoffset))
            break;

        State substate;
        substate.action = "mouse_scroll";
        substate.value = Values({xoffset, yoffset});
        substate.window = getWindowName(win);
        _state.emplace_back(std::move(substate));
    }
}

} // end of namespace
