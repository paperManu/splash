#include "./controller/controller_gui.h"

#include <fstream>
#include <memory>
#include <optional>

#include "./controller/controller.h"
#include "./controller/widget/widget_calibration.h"
#include "./controller/widget/widget_camera.h"
#include "./controller/widget/widget_control.h"
#include "./controller/widget/widget_filters.h"
#include "./controller/widget/widget_graph.h"
#include "./controller/widget/widget_media.h"
#include "./controller/widget/widget_meshes.h"
#include "./controller/widget/widget_node_view.h"
#include "./controller/widget/widget_text_box.h"
#include "./controller/widget/widget_textures_view.h"
#include "./controller/widget/widget_tree.h"
#include "./controller/widget/widget_warp.h"
#include "./core/constants.h"
#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/window.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

#define GUI_WINDOW_NAME "__gui_window"

namespace Splash
{

/**
 * Convert GLFW keycodes to ImGui
 *
 * \param keycode GLFW keycode
 * \return Return ImGui keycode
 */
ImGuiKey ImGuiGLFWToImGuiKey(int keycode)
{
    switch (keycode)
    {
    case GLFW_KEY_TAB:
        return ImGuiKey_Tab;
    case GLFW_KEY_LEFT:
        return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT:
        return ImGuiKey_RightArrow;
    case GLFW_KEY_UP:
        return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN:
        return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP:
        return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN:
        return ImGuiKey_PageDown;
    case GLFW_KEY_HOME:
        return ImGuiKey_Home;
    case GLFW_KEY_END:
        return ImGuiKey_End;
    case GLFW_KEY_INSERT:
        return ImGuiKey_Insert;
    case GLFW_KEY_DELETE:
        return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE:
        return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE:
        return ImGuiKey_Space;
    case GLFW_KEY_ENTER:
        return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE:
        return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA:
        return ImGuiKey_Comma;
    case GLFW_KEY_MINUS:
        return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD:
        return ImGuiKey_Period;
    case GLFW_KEY_SLASH:
        return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON:
        return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL:
        return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET:
        return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH:
        return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET:
        return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
        return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK:
        return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK:
        return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK:
        return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN:
        return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE:
        return ImGuiKey_Pause;
    case GLFW_KEY_KP_0:
        return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1:
        return ImGuiKey_Keypad1;
    case GLFW_KEY_KP_2:
        return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3:
        return ImGuiKey_Keypad3;
    case GLFW_KEY_KP_4:
        return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5:
        return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6:
        return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7:
        return ImGuiKey_Keypad7;
    case GLFW_KEY_KP_8:
        return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9:
        return ImGuiKey_Keypad9;
    case GLFW_KEY_KP_DECIMAL:
        return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD:
        return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER:
        return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL:
        return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT:
        return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL:
        return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT:
        return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER:
        return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT:
        return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL:
        return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT:
        return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER:
        return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU:
        return ImGuiKey_Menu;
    case GLFW_KEY_0:
        return ImGuiKey_0;
    case GLFW_KEY_1:
        return ImGuiKey_1;
    case GLFW_KEY_2:
        return ImGuiKey_2;
    case GLFW_KEY_3:
        return ImGuiKey_3;
    case GLFW_KEY_4:
        return ImGuiKey_4;
    case GLFW_KEY_5:
        return ImGuiKey_5;
    case GLFW_KEY_6:
        return ImGuiKey_6;
    case GLFW_KEY_7:
        return ImGuiKey_7;
    case GLFW_KEY_8:
        return ImGuiKey_8;
    case GLFW_KEY_9:
        return ImGuiKey_9;
    case GLFW_KEY_A:
        return ImGuiKey_A;
    case GLFW_KEY_B:
        return ImGuiKey_B;
    case GLFW_KEY_C:
        return ImGuiKey_C;
    case GLFW_KEY_D:
        return ImGuiKey_D;
    case GLFW_KEY_E:
        return ImGuiKey_E;
    case GLFW_KEY_F:
        return ImGuiKey_F;
    case GLFW_KEY_G:
        return ImGuiKey_G;
    case GLFW_KEY_H:
        return ImGuiKey_H;
    case GLFW_KEY_I:
        return ImGuiKey_I;
    case GLFW_KEY_J:
        return ImGuiKey_J;
    case GLFW_KEY_K:
        return ImGuiKey_K;
    case GLFW_KEY_L:
        return ImGuiKey_L;
    case GLFW_KEY_M:
        return ImGuiKey_M;
    case GLFW_KEY_N:
        return ImGuiKey_N;
    case GLFW_KEY_O:
        return ImGuiKey_O;
    case GLFW_KEY_P:
        return ImGuiKey_P;
    case GLFW_KEY_Q:
        return ImGuiKey_Q;
    case GLFW_KEY_R:
        return ImGuiKey_R;
    case GLFW_KEY_S:
        return ImGuiKey_S;
    case GLFW_KEY_T:
        return ImGuiKey_T;
    case GLFW_KEY_U:
        return ImGuiKey_U;
    case GLFW_KEY_V:
        return ImGuiKey_V;
    case GLFW_KEY_W:
        return ImGuiKey_W;
    case GLFW_KEY_X:
        return ImGuiKey_X;
    case GLFW_KEY_Y:
        return ImGuiKey_Y;
    case GLFW_KEY_Z:
        return ImGuiKey_Z;
    case GLFW_KEY_F1:
        return ImGuiKey_F1;
    case GLFW_KEY_F2:
        return ImGuiKey_F2;
    case GLFW_KEY_F3:
        return ImGuiKey_F3;
    case GLFW_KEY_F4:
        return ImGuiKey_F4;
    case GLFW_KEY_F5:
        return ImGuiKey_F5;
    case GLFW_KEY_F6:
        return ImGuiKey_F6;
    case GLFW_KEY_F7:
        return ImGuiKey_F7;
    case GLFW_KEY_F8:
        return ImGuiKey_F8;
    case GLFW_KEY_F9:
        return ImGuiKey_F9;
    case GLFW_KEY_F10:
        return ImGuiKey_F10;
    case GLFW_KEY_F11:
        return ImGuiKey_F11;
    case GLFW_KEY_F12:
        return ImGuiKey_F12;
    case GLFW_KEY_F13:
        return ImGuiKey_F13;
    case GLFW_KEY_F14:
        return ImGuiKey_F14;
    case GLFW_KEY_F15:
        return ImGuiKey_F15;
    case GLFW_KEY_F16:
        return ImGuiKey_F16;
    case GLFW_KEY_F17:
        return ImGuiKey_F17;
    case GLFW_KEY_F18:
        return ImGuiKey_F18;
    case GLFW_KEY_F19:
        return ImGuiKey_F19;
    case GLFW_KEY_F20:
        return ImGuiKey_F20;
    case GLFW_KEY_F21:
        return ImGuiKey_F21;
    case GLFW_KEY_F22:
        return ImGuiKey_F22;
    case GLFW_KEY_F23:
        return ImGuiKey_F23;
    case GLFW_KEY_F24:
        return ImGuiKey_F24;
    default:
        return ImGuiKey_None;
    }
}

/*************/
Gui::Gui(RenderingContext* renderingContext, RootObject* root)
    : ControllerObject(root)
{
    _type = "gui";
    _renderingPriority = Priority::GUI;

    if (renderingContext == nullptr || _scene == nullptr)
        return;

    _renderingContext = renderingContext;

    _guiGfxImpl = _renderer->createGuiGfxImpl();
    _fbo = _renderer->createFramebuffer();
    _fbo->setResizable(true);

    // Intialize the GUI widgets
    ImGui::CreateContext();
    initImGui(_width, _height);
    initImWidgets();
    loadIcon();

    registerAttributes();

    // Callback for dragndrop: load the dropped file
    UserInput::setCallback(UserInput::State("dragndrop"), [this](const UserInput::State& state) { setWorldAttribute("loadConfig", {state.value[0].as<std::string>()}); });
}

/*************/
Gui::~Gui()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Gui::~Gui - Destructor" << Log::endl;
#endif

    // Clean ImGui
    ImGui::DestroyContext();
}

/*************/
void Gui::loadIcon()
{
    auto imagePath = std::string(DATADIR);
    std::string path{"splash.png"};

    auto image = std::make_shared<Image>(_scene);
    image->setName("splash_icon");
    if (!image->read(imagePath + path))
    {
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - Could not find Splash icon, aborting image loading" << Log::endl;
        return;
    }

    _splashLogo = std::make_shared<Texture_Image>(_scene);
    _splashLogo->linkTo(image);
    _splashLogo->update();
}

/*************/
void Gui::unicodeChar(unsigned int unicodeChar)
{
    using namespace ImGui;
    ImGuiIO& io = GetIO();
    if (unicodeChar > 0 && unicodeChar < 0x10000)
        io.AddInputCharacter((unsigned short)unicodeChar);
}

/*************/
void Gui::computeBlending(bool once)
{
    auto blendingState = getObjectAttribute("blender", "mode");
    std::string previousState = "none";
    if (!blendingState.empty())
        previousState = blendingState[0].as<std::string>();

    if (previousState != "none")
    {
        setObjectAttribute("blender", "mode", {"none"});
        _blendingActive = false;
    }
    else if (once)
    {
        setObjectAttribute("blender", "mode", {"once"});
        _blendingActive = true;
    }
    else
    {
        setObjectAttribute("blender", "mode", {"continuous"});
        _blendingActive = true;
    }
}

/*************/
void Gui::calibrateColorResponseFunction()
{
    if (!_scene)
        return;
    _scene->setAttribute("calibrateColorResponseFunction", {});
}

/*************/
void Gui::calibrateColors()
{
    if (!_scene)
        return;
    _scene->setAttribute("calibrateColor", {});
}

/*************/
void Gui::copyCameraParameters(const std::string& path)
{
    setWorldAttribute("copyCameraParameters", {path});
}

/*************/
void Gui::setJoystick(const std::vector<float>& axes, const std::vector<uint8_t>& buttons)
{
    for (auto& w : _guiWidgets)
        w->setJoystick(axes, buttons);
}

/*************/
void Gui::setJoystickState(const std::vector<UserInput::State>& state)
{
    std::vector<float> axes;
    std::vector<uint8_t> buttons;

    for (const auto& s : state)
    {
        if (s.action == "joystick_0_axes")
            for (const auto& v : s.value)
                axes.push_back(v.as<float>());
        else if (s.action == "joystick_0_buttons")
            for (const auto& v : s.value)
                buttons.push_back(v.as<int>());
    }

    setJoystick(axes, buttons);
}

/*************/
void Gui::setKeyboardState(const std::vector<UserInput::State>& state)
{
    for (const auto& s : state)
    {
        if (s.action == "keyboard_unicodeChar")
        {
            uint32_t unicode = static_cast<uint32_t>(s.value[0].as<int>());
            unicodeChar(unicode);
            continue;
        }

        auto key = s.value[0].as<int>();
        auto mods = s.modifiers;

        if (s.action == "keyboard_press")
            Gui::key(key, GLFW_PRESS, mods);
        else if (s.action == "keyboard_release")
            Gui::key(key, GLFW_RELEASE, mods);
        else if (s.action == "keyboard_repeat")
            Gui::key(key, GLFW_REPEAT, mods);
    }
}

/*************/
void Gui::setMouseState(const std::vector<UserInput::State>& state)
{
    if (!_renderingContext)
        return;

    for (const auto& s : state)
    {
        auto parentIt = find_if(_parents.begin(), _parents.end(), [&](const GraphObject* p) { return s.window == p->getName(); });
        if (parentIt == _parents.end())
        {
            _mouseHoveringWindow = false;
        }
        else
        {
            _mouseHoveringWindow = true;

            if (s.action == "mouse_position")
                mousePosition(s.value[0].as<int>(), s.value[1].as<int>());
            else if (s.action == "mouse_press")
                mouseButton(s.value[0].as<int>(), GLFW_PRESS, s.modifiers);
            else if (s.action == "mouse_release")
                mouseButton(s.value[0].as<int>(), GLFW_RELEASE, s.modifiers);
            else if (s.action == "mouse_scroll")
                mouseScroll(s.value[0].as<float>(), s.value[1].as<float>());
        }
    }
}

/*************/
void Gui::key(int key, int action, int mods)
{
    switch (key)
    {
    default:
    {
    sendAsDefault:
        using namespace ImGui;

        // Numpad enter is converted to regular enter
        if (key == GLFW_KEY_KP_ENTER)
            key = GLFW_KEY_ENTER;

        ImGuiIO& io = GetIO();
        const ImGuiKey imguiKey = ImGuiGLFWToImGuiKey(key);
        io.AddKeyEvent(imguiKey, (action == GLFW_PRESS || action == GLFW_REPEAT));
        io.AddKeyEvent(ImGuiMod_Ctrl, ((mods & GLFW_MOD_CONTROL) != 0) && (action == GLFW_PRESS || action == GLFW_REPEAT));
        io.AddKeyEvent(ImGuiMod_Shift, ((mods & GLFW_MOD_SHIFT) != 0) && (action == GLFW_PRESS || action == GLFW_REPEAT));

        break;
    }
    case GLFW_KEY_TAB:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            if (!_fullscreen)
                _isVisible = !_isVisible;
        }
        else if (action == GLFW_PRESS && mods == GLFW_MOD_SHIFT)
            toggleGuiDocking();
        else
            goto sendAsDefault;
        break;
    }
    case GLFW_KEY_B:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
            computeBlending(true);
        else if (action == GLFW_PRESS && mods == (GLFW_MOD_CONTROL + GLFW_MOD_ALT))
            computeBlending(false);
        break;
    }
    case GLFW_KEY_F:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
            setObjectsOfType("camera", "flashBG", {});
        break;
    }
    case GLFW_KEY_M:
    {
        if (mods == GLFW_MOD_CONTROL && action == GLFW_PRESS)
        {
            static bool cursorVisible = false;
            cursorVisible = !cursorVisible;

            setObjectsOfType("window", "showCursor", {(int)cursorVisible});
        }
        break;
    }
    case GLFW_KEY_O:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            _showFileSelector = true;
            _menuAction = MenuAction::OpenConfiguration;
        }
        else if (action == GLFW_PRESS && mods == (GLFW_MOD_CONTROL + GLFW_MOD_SHIFT))
        {
            _showFileSelector = true;
            _menuAction = MenuAction::OpenProject;
        }
        break;
    }
    case GLFW_KEY_S:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            setWorldAttribute("save", {_configurationPath});
        }
        else if (action == GLFW_PRESS && mods == (GLFW_MOD_CONTROL + GLFW_MOD_SHIFT))
        {
            _showFileSelector = true;
            _menuAction = MenuAction::SaveConfigurationAs;
        }
        break;
    }
    case GLFW_KEY_T:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            _wireframe = false;
            setWorldAttribute("wireframe", {_wireframe});
        }
        break;
    }
    // Switch the rendering to wireframe
    case GLFW_KEY_W:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            _wireframe = true;
            setWorldAttribute("wireframe", {_wireframe});
        }
        break;
    }
    }
}

/*************/
void Gui::mousePosition(int xpos, int ypos)
{
    using namespace ImGui;
    ImGuiIO& io = GetIO();

    io.MousePos = ImVec2((float)xpos, (float)ypos);

    return;
}

/*************/
void Gui::mouseButton(int btn, int action, int mods)
{
    using namespace ImGui;
    ImGuiIO& io = GetIO();

    io.AddKeyEvent(ImGuiMod_Ctrl, ((mods & GLFW_MOD_CONTROL) != 0));
    io.AddKeyEvent(ImGuiMod_Shift, ((mods & GLFW_MOD_SHIFT) != 0));
    io.AddKeyEvent(ImGuiMod_Alt, ((mods & GLFW_MOD_ALT) != 0));

    bool isPressed = action == GLFW_PRESS ? true : false;
    switch (btn)
    {
    default:
        break;
    case GLFW_MOUSE_BUTTON_LEFT:
        io.MouseDown[0] = isPressed;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (io.KeyAlt)
            io.MouseDown[2] = isPressed;
        else
            io.MouseDown[1] = isPressed;
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        io.MouseDown[2] = isPressed;
        break;
    }

    return;
}

/*************/
void Gui::mouseScroll(double /*xoffset*/, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += (float)yoffset;

    return;
}

/*************/
bool Gui::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Object>(obj))
    {
        auto object = std::dynamic_pointer_cast<Object>(obj);
        for (auto& widget : _guiWidgets)
            widget->linkTo(object);
        for (auto& widget : _guiBottomWidgets)
            widget->linkTo(object);

        return true;
    }

    return false;
}

/*************/
void Gui::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        for (auto& widget : _guiWidgets)
            widget->unlinkFrom(obj);
        for (auto& widget : _guiBottomWidgets)
            widget->unlinkFrom(obj);
    }
}

/*************/
void Gui::drawMainTab()
{
    ImGui::BeginChild("main");

    ImVec2 availableSize = ImGui::GetContentRegionAvail();

    ImGui::Text("General");
    if (ImGui::Button("Compute blending map", ImVec2(availableSize[0] / 3.f, 32.f)))
        computeBlending(true);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("To use once cameras are calibrated (Ctrl+%s, Ctrl+Alt+%s to compute at each frames)", getLocalKeyName('B'), getLocalKeyName('B'));

    ImGui::SameLine();
    if (ImGui::Button("Flash background", ImVec2(availableSize[0] / 3.f, 32.f)))
        setObjectsOfType("camera", "flashBG", {});
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Set the background as light gray (Ctrl+%s)", getLocalKeyName('F'));

    ImGui::SameLine();
    if (ImGui::Button("Wireframe / Textured", ImVec2(availableSize[0] / 3.f, 32.f)))
    {
        _wireframe = !_wireframe;
        setWorldAttribute("wireframe", {_wireframe});
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Switch objects between wireframe and textured (Ctrl+%s and Ctrl+%s)", getLocalKeyName('T'), getLocalKeyName('W'));

#if HAVE_GPHOTO and HAVE_OPENCV
    ImGui::Separator();
    ImGui::Text("Color calibration");
    if (ImGui::Button("Calibrate camera response", ImVec2(availableSize[0] / 3.f, 32.f)))
        calibrateColorResponseFunction();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Needs to be done before projector calibration");

    ImGui::SameLine();
    if (ImGui::Button("Calibrate displays / projectors", ImVec2(availableSize[0] / 3.f, 32.f)))
        calibrateColors();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Calibrates all outputs");

    ImGui::SameLine();
    if (ImGui::Button("Activate correction", ImVec2(availableSize[0] / 3.f, 32.f)))
    {
        auto cameras = getObjectsPtr(getObjectsOfType("camera"));
        for (auto& cam : cameras)
        {
            setObjectAttribute(cam->getName(), "activateColorLUT", {2});
            setObjectAttribute(cam->getName(), "activateColorMixMatrix", {2});
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Activate color LUT, once calibrated");

#endif

    // Media directory
    ImGui::Separator();
    ImGui::Text("Media directory");
    std::string mediaPath = _root->getMediaPath();
    if (SplashImGui::InputText("##MediaPath", mediaPath))
        setWorldAttribute("mediaPath", {std::string(mediaPath)});

    ImGui::SameLine();
    static bool showMediaFileSelector{false};
    if (ImGui::Button("...##Media"))
        showMediaFileSelector = true;
    if (showMediaFileSelector)
    {
        static std::string path = _root->getMediaPath();
        bool cancelled;
        if (SplashImGui::FileSelector("Media path", path, cancelled, {{"json"}}))
        {
            if (!cancelled)
            {
                path = Utils::getPathFromFilePath(path);
                setWorldAttribute("mediaPath", {Utils::getPathFromFilePath(path)});
            }
            else
            {
                path = _root->getMediaPath();
            }
            showMediaFileSelector = false;
        }
    }

#if HAVE_PORTAUDIO
    ImGui::Separator();
    ImGui::Text("Master/LTC clock");
    auto clockDeviceValue = getWorldAttribute("clockDeviceName");
    if (!clockDeviceValue.empty())
    {
        auto clockDeviceName = clockDeviceValue[0].as<std::string>();
        SplashImGui::InputText("##clockDeviceName", clockDeviceName);
        if (ImGui::IsItemDeactivatedAfterEdit())
            setWorldAttribute("clockDeviceName", {clockDeviceName});
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("If a JACK audio server is used, specify the input device name to read the LTC clock from.\nOtherwise the default Pulseaudio input is used.");
        ImGui::SameLine();
        ImGui::Text("JACK audio input device");
    }

    static auto looseClock = false;
    auto looseClockValue = getWorldAttribute("looseClock");
    if (!looseClockValue.empty())
    {
        looseClock = looseClockValue[0].as<bool>();
        if (ImGui::Checkbox("Loose master clock", &looseClock))
            setWorldAttribute("looseClock", {looseClock});
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Loose clock: if activated, the master clock is only "
                              "used as an indication, not a hard constraint");
    }
#endif

    ImGui::Separator();
    ImGui::Text("Blending parameters");
    static auto blendWidth = 0.05f;
    ImGui::InputFloat("Blending width", &blendWidth, 0.01f, 0.04f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit())
        setObjectsOfType("camera", "blendWidth", {blendWidth});

    static auto blendPrecision = 0.1f;
    ImGui::InputFloat("Blending precision", &blendPrecision, 0.01f, 0.04f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit())
        setObjectsOfType("camera", "blendPrecision", {blendPrecision});

    auto depthAwareBlendingValue = getObjectAttribute("blender", "depthAwareBlending");
    if (!depthAwareBlendingValue.empty())
    {
        auto depthAwareBlending = depthAwareBlendingValue[0].as<bool>();
        if (ImGui::Checkbox("Activate depth-aware blending", &depthAwareBlending))
            setObjectAttribute("blender", "depthAwareBlending", {depthAwareBlending});
        if (ImGui::IsItemHovered())
        {
            auto description = getObjectAttributeDescription("blender", "depthAwareBlending");
            if (!description.empty())
                ImGui::SetTooltip("%s", description[0].as<std::string>().c_str());
        }
    }

    ImGui::Separator();
    ImGui::Text("Testing tools");

    static auto syncTestFrameDelay = 0;
    ImGui::InputInt("Outputs synchronization test", &syncTestFrameDelay, 1, 100);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        syncTestFrameDelay = std::max(syncTestFrameDelay, 0);
        setWorldAttribute("swapTest", {syncTestFrameDelay});
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Test frame swap synchronization by setting a black/white frames swap with the given period");

    static auto showCameraCount = false;
    if (ImGui::Checkbox("Show camera count (w/ blending)", &showCameraCount))
    {
        setWorldAttribute("computeBlending", {"continuous"});
        setObjectsOfType("camera", "showCameraCount", {showCameraCount});
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Replace the objects texture with a color indicating the number of cameras displaying each pixel");

    ImGui::EndChild();
}

/*************/
void Gui::drawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            static std::string menuOpenConfig = std::string("Open configuration (Ctrl+") + getLocalKeyName('O') + ")";
            if (ImGui::MenuItem(menuOpenConfig.c_str(), nullptr))
            {
                _menuAction = MenuAction::OpenConfiguration;
                _showFileSelector = true;
            }
            ImGui::Separator();
            static std::string menuSaveConfiguration = std::string("Save configuration (Ctrl+") + getLocalKeyName('S') + ")";
            if (ImGui::MenuItem(menuSaveConfiguration.c_str(), nullptr))
            {
                setWorldAttribute("save", {_configurationPath});
            }
            static std::string menuSaveConfigurationAs = std::string("Save configuration as... (Ctrl+Shift+") + getLocalKeyName('S') + ")";
            if (ImGui::MenuItem(menuSaveConfigurationAs.c_str(), nullptr))
            {
                _menuAction = MenuAction::SaveConfigurationAs;
                _showFileSelector = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy calibration from configuration", nullptr))
            {
                _menuAction = MenuAction::CopyCalibration;
                _showFileSelector = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", nullptr))
            {
                setWorldAttribute("quit", {});
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project"))
        {
            static std::string menuOpenProject = std::string("Open project (Ctrl+Shift+") + getLocalKeyName('O') + ")";
            if (ImGui::MenuItem(menuOpenProject.c_str(), nullptr))
            {
                _menuAction = MenuAction::OpenProject;
                _showFileSelector = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save project", nullptr))
            {
                setWorldAttribute("saveProject", {_projectPath});
            }
            if (ImGui::MenuItem("Save project as ...", nullptr))
            {
                _menuAction = MenuAction::SaveProjectAs;
                _showFileSelector = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Toggle GUI docking (Shift+Tab)", nullptr))
            {
                toggleGuiDocking();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Shortcuts", nullptr))
            {
                _showHelp = true;
            }
            if (ImGui::MenuItem("About", nullptr))
            {
                _showAbout = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (_showFileSelector)
    {
        static std::string path = _root->getConfigurationPath();
        bool cancelled;

        if (_menuAction == MenuAction::OpenConfiguration)
        {
            if (SplashImGui::FileSelector("Open configuration", path, cancelled, {{".json"}}))
            {
                _showFileSelector = false;
                if (!cancelled)
                {
                    _configurationPath = path;
                    setWorldAttribute("loadConfig", {_configurationPath});
                }
            }
        }
        else if (_menuAction == MenuAction::OpenProject)
        {
            if (SplashImGui::FileSelector("Open project", path, cancelled, {{".json"}}))
            {
                _showFileSelector = false;
                if (!cancelled)
                {
                    _projectPath = path;
                    setWorldAttribute("loadProject", {_projectPath});
                }
            }
        }
        else if (_menuAction == MenuAction::CopyCalibration)
        {
            if (SplashImGui::FileSelector("Copy calibration from...", path, cancelled, {{".json"}}))
            {
                _showFileSelector = false;
                if (!cancelled)
                    copyCameraParameters(path);
            }
        }
        else if (_menuAction == MenuAction::SaveConfigurationAs)
        {
            if (SplashImGui::FileSelector("Save configuration as...", path, cancelled, {{".json"}}, true, true))
            {
                _showFileSelector = false;
                if (!cancelled)
                {
                    _configurationPath = path;
                    setWorldAttribute("save", {_configurationPath});
                }
            }
        }
        else if (_menuAction == MenuAction::SaveProjectAs)
        {
            if (SplashImGui::FileSelector("Save project as...", path, cancelled, {{".json"}}, true, true))
            {
                _showFileSelector = false;
                if (!cancelled)
                {
                    _projectPath = path;
                    setWorldAttribute("saveProject", {_projectPath});
                }
            }
        }
        else
        {
            assert(false);
        }
    }
}

/*************/
void Gui::renderSplashScreen()
{
    static bool isOpen{false};
    int splashWidth = 600, splashHeight = 288;
    auto parentWindowSize = ImGui::GetWindowSize();
    auto parentWindowPos = ImGui::GetWindowPos();
    ImGui::SetNextWindowPos(ImVec2(parentWindowPos[0] + (parentWindowSize[0] - splashWidth) / 2.f, parentWindowPos[1] + (parentWindowSize[1] - splashHeight) / 2.f));
    ImGui::SetNextWindowSize(ImVec2(splashWidth, splashHeight));
    ImGui::Begin("About Splash", &isOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    if (_splashLogo)
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::Image((ImTextureID)(intptr_t)_splashLogo->getTexId(), ImVec2(256, 256));
        ImGui::NextColumn();
        ImGui::Dummy(ImVec2(256, 80));
        ImGui::Text("Splash, a modular video-mapping engine");
        ImGui::Text("Version %s", PACKAGE_VERSION);
        ImGui::Spacing();
        ImGui::Text("https://splashmapper.xyz");
        ImGui::Columns(1);

        auto& io = ImGui::GetIO();
        if (io.MouseClicked[0])
            _showAbout = false;
    }
    ImGui::End();
}

/*************/
void Gui::renderHelp()
{

    static bool isOpen{false};
    int helpWidth = 400, helpHeight = 400;
    auto parentWindowSize = ImGui::GetWindowSize();
    auto parentWindowPos = ImGui::GetWindowPos();
    ImGui::SetNextWindowPos(ImVec2(parentWindowPos[0] + (parentWindowSize[0] - helpWidth) / 2.f, parentWindowPos[1] + 100.f));
    ImGui::SetNextWindowSize(ImVec2(helpWidth, helpHeight));
    ImGui::Begin("Help", &isOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Ctrl+Tab: show / hide this GUI");
    ImGui::Text("Shift+Tab: toggle setting this GUI to its own window");
    ImGui::Text("General shortcuts:");
    ImGui::Text("  Ctrl+%s: open a configuration", getLocalKeyName('O'));
    ImGui::Text("  Ctrl+Shift+%s: open a project", getLocalKeyName('O'));
    ImGui::Text("  Ctrl+%s: save the current configuration", getLocalKeyName('S'));
    ImGui::Text("  Ctrl+Shift+%s: save configuration as...", getLocalKeyName('S'));
    ImGui::Text("  Ctrl+%s: white background instead of black", getLocalKeyName('F'));
    ImGui::Text("  Ctrl+%s: compute the blending between all projectors", getLocalKeyName('B'));
    ImGui::Text("  Ctrl+Alt+%s: compute the blending between all projectors at every frame", getLocalKeyName('B'));
    ImGui::Text("  Ctrl+%s: hide/show the OS cursor", getLocalKeyName('M'));
    ImGui::Text("  Ctrl+%s: textured draw mode", getLocalKeyName('T'));
    ImGui::Text("  Ctrl+%s: wireframe draw mode", getLocalKeyName('W'));
    ImGui::Text(" ");
    ImGui::Text("Views panel:");
    ImGui::Text("  Ctrl + left click on a camera thumbnail: hide / show the given camera");
    ImGui::Text("  Space: switch between projectors");
    ImGui::Text("  %s: show / hide the target calibration point", getLocalKeyName('A'));
    ImGui::Text("  %s: calibrate the selected camera", getLocalKeyName('C'));
    ImGui::Text("  %s: hide all but the selected camera", getLocalKeyName('H'));
    ImGui::Text("  %s: show calibration points from all cameras", getLocalKeyName('O'));
    ImGui::Text("  Ctrl+%s: revert camera to previous calibration", getLocalKeyName('Z'));
    ImGui::Text(" ");
    ImGui::Text("Node view (inside Control panel):");
    ImGui::Text("  Shift + left click: link the clicked node to the selected one");
    ImGui::Text("  Ctrl + left click: unlink the clicked node from the selected one");
    ImGui::Text(" ");
    ImGui::Text("Camera view (inside the Camera panel):");
    ImGui::Text("  Middle click: rotate camera");
    ImGui::Text("  Shift + middle click: pan camera");
    ImGui::Text("  Control + middle click: zoom in / out");
    ImGui::Text(" ");
    ImGui::Text("Joystick controls (may vary with the controller):");
    ImGui::Text("  Directions: move the selected calibration point");
    ImGui::Text("  Button 1: select previous calibration point");
    ImGui::Text("  Button 2: select next calibration point");
    ImGui::Text("  Button 3: hide all but the selected cameras");
    ImGui::Text("  Button 4: calibrate");
    ImGui::Text("  Button 5: move calibration point slower");
    ImGui::Text("  Button 6: move calibration point faster");
    ImGui::Text("  Button 7: flash the background to light gray");

    auto& io = ImGui::GetIO();
    if (io.MouseClicked[0])
        _showHelp = false;

    ImGui::End();
}

/*************/
void Gui::toggleGuiDocking()
{
    // It can happen that no window is currently linked, for
    // example when a self-owned window has been asked to be created
    // by the updateGuiWindow method.
    // If that is the case, we do nothing for now.
    if (!_window)
        return;

    if (_window != _selfWindow.get())
    {
        setInScene("unlink", {_name, _window->getName()});
    }
    else if (_selfWindow)
    {
        const auto windowNames = getObjectsOfType("window");
        const auto windows = getObjectsPtr(windowNames);
        for (auto& window : windows)
        {
            if (window->getName() == _selfWindow->getName())
                continue;
            setInScene("link", {_name, window->getName()});
            break;
        }
    }

    // Make sure the GUI is visible
    _isVisible = true;
}

/*************/
void Gui::updateGuiWindow()
{
    if (!_window && !_selfWindow && !_creatingWindow)
    {
        // If no window exist for this GUI, and none is currently being created
        setWorldAttribute("addObject", {"window", GUI_WINDOW_NAME, _scene->getName()});
        _creatingWindow = true;
    }
    else if (_creatingWindow)
    {
        // If a window is currently being created
        auto window = std::dynamic_pointer_cast<Window>(getObjectPtr(GUI_WINDOW_NAME));
        if (!window)
            return;
        _selfWindow = window;
        _selfWindow->setSavable(false);
        setWorldAttribute("link", {_name, GUI_WINDOW_NAME});
        _fullscreen = true;
        _isVisible = true;
        _creatingWindow = false;
    }
    else if (_selfWindow && _window && _window != _selfWindow.get())
    {
        // If the window linked is not the same as the self-owned window
        // This means the GUI was previously shown in the self-owned window,
        // and that this changed recently.
        setWorldAttribute("deleteObject", {GUI_WINDOW_NAME});
        _selfWindow.reset();
        _fullscreen = false;
        _isVisible = true;
    }
}

/*************/
void Gui::render()
{
    if (!_isInitialized)
        return;

    ImageBufferSpec spec = _fbo->getColorTexture()->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);

    // If not linked to any Window, create our own one
    updateGuiWindow();

    // Let's start the UI for real
    using namespace ImGui;

    ImGuiIO& io = GetIO();
    io.MouseDrawCursor = _mouseHoveringWindow;

    ImGui::NewFrame();

    // Panel
    if (_isVisible)
    {
        std::string windowName = "Splash Control Panel";
        if (!_configurationPath.empty())
            windowName += " - " + _configurationPath;
        ImGui::SetNextWindowSize(ImVec2(_defaultWidth, _defaultHeight), ImGuiCond_Once);
        ImGui::SetNextWindowBgAlpha(_backgroundAlpha);
        ImGui::Begin(windowName.c_str(), nullptr, _windowFlags);
        _windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;

        if (_showAbout)
            renderSplashScreen();

        if (_showHelp)
            renderHelp();

        // Check whether the GUI is alone in its window
        auto objReversedLinks = getObjectReversedLinks();
        auto objLinks = getObjectLinks();
        if (_fullscreen)
        {
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::SetWindowSize(ImVec2(_width, _height));
            _windowFlags |= ImGuiWindowFlags_NoMove;
        }
        else
        {
            ImGui::SetWindowPos(ImVec2(_initialGuiPos[0], _initialGuiPos[1]), ImGuiCond_Once);
        }

        drawMenuBar();

        std::optional<std::string> activeObjectName;
        ImGui::BeginChild("##controlWidgets", ImVec2(0, -256));
        ImGui::BeginChild("##tabulations", ImVec2(-384, 0));
        if (ImGui::BeginTabBar("Tabs", ImGuiTabBarFlags_None))
        {
            // Controls tabulations
            for (auto& widget : _guiWidgets)
            {
                widget->update();
                if (ImGui::BeginTabItem(widget->getName().c_str()))
                {
                    ImGui::BeginChild(widget->getName().c_str());
                    widget->render();
                    activeObjectName = widget->getActiveObjectName();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                _windowFlags |= widget->updateWindowFlags();
            }

            // Options tabulation
            if (ImGui::BeginTabItem("Options"))
            {
                drawMainTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        if (activeObjectName)
        {
            ImGui::SameLine();
            ImGui::BeginChild("##objectAttributes");
            _guiAttributes->setTargetObjectName(activeObjectName.value());
            _guiAttributes->render();
            ImGui::EndChild();
        }
        ImGui::EndChild();

        // Health information
        if (!_guiBottomWidgets.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BeginChild("##bottomWidgets", ImVec2(0, -44));
            ImVec2 availableSize = ImGui::GetContentRegionAvail();
            for (auto& widget : _guiBottomWidgets)
            {
                widget->update();
                ImGui::BeginChild(widget->getName().c_str(), ImVec2(availableSize.x / static_cast<float>(_guiBottomWidgets.size()), 0), ImGuiChildFlags_Borders);
                widget->render();
                ImGui::EndChild();
                ImGui::SameLine();
            }
            ImGui::EndChild();
        }

#if HAVE_PORTAUDIO
        // Master clock
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("##masterClock", ImVec2(0, 0));
        {

            int year{0}, month{0}, day{0}, hour{0}, minute{0}, second{0}, frame{0};
            bool paused{true};

            auto tree = _root->getTree();
            Value clock;
            if (tree->getValueForLeafAt("/world/attributes/masterClock", clock) && clock.size() == 8)
            {
                year = clock[0].as<int>();
                month = clock[1].as<int>();
                day = clock[2].as<int>();
                hour = clock[3].as<int>();
                minute = clock[4].as<int>();
                second = clock[5].as<int>();
                frame = clock[6].as<int>();
                paused = clock[7].as<bool>();
            }

            std::ostringstream stream;
            stream << std::setfill('0') << std::setw(2) << year;
            stream << "/" << std::setfill('0') << std::setw(2) << month;
            stream << "/" << std::setfill('0') << std::setw(2) << day;
            stream << " - " << std::setfill('0') << std::setw(2) << hour;
            stream << ":" << std::setfill('0') << std::setw(2) << minute;
            stream << ":" << std::setfill('0') << std::setw(2) << second;
            stream << ":" << std::setfill('0') << std::setw(3) << frame;
            stream << (paused ? " - Paused" : " - Playing");

            ImGui::PushFont(_guiFonts[FontType::Clock]);
            ImGui::Text("%s", stream.str().c_str());
            ImGui::PopFont();
        }
        ImGui::EndChild();
#endif

        ImGui::End();
    }

    // Uncomment to show styling gui!
    // ImGuiStyle& style = ImGui::GetStyle();
    // ImGui::ShowStyleEditor(&style);

    static double time = 0.0;
    const double currentTime = glfwGetTime();
    io.DeltaTime = static_cast<float>(currentTime - time);
    time = currentTime;

    if (_isVisible || _wasVisible || _resized)
    {
        _wasVisible = _isVisible;

        _fbo->bindDraw();
        _guiGfxImpl->setupViewport(static_cast<uint32_t>(io.DisplaySize.x), static_cast<uint32_t>(io.DisplaySize.y));
        if (_isVisible)
        {
            ImGui::Render();
            const float width = ImGui::GetIO().DisplaySize.x;
            const float height = ImGui::GetIO().DisplaySize.y;
            _guiGfxImpl->drawGui(width, height, ImGui::GetDrawData());
        }
        _fbo->unbindDraw();
    }

    ImGui::EndFrame();
    _fbo->getColorTexture()->generateMipmap();
    _resized = false;

    return;
}

/*************/
void Gui::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    const auto isContextActive = _renderingContext->isCurrentContext();
    if (!isContextActive)
        _renderingContext->setAsCurrentContext();

    _fbo->setSize(width, height);

    _width = width;
    _height = height;
    _resized = true;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;

    if (!isContextActive)
        _renderingContext->releaseContext();
}

/*************/
void Gui::initImGui(int width, int height)
{
    using namespace ImGui;

    _guiGfxImpl->initRendering();

    // Initialize ImGui
    ImGuiIO& io = GetIO();

    // Fonts
    static const std::vector<FontDefinition> fonts{{FontType::Default, "Roboto-Medium.ttf", 15}, {FontType::Clock, "DSEG14Classic-Light.ttf", 20}};
    for (const auto& font : fonts)
    {
        assert(font.size > 0);

        auto fullPath = std::string(DATADIR) + "../fonts/" + font.filename;
        if (std::ifstream(fullPath, std::ios::in | std::ios::binary))
        {
            _guiFonts[font.type] = io.Fonts->AddFontFromFileTTF(fullPath.c_str(), font.size);
        }
    }
    io.Fonts->Build();

    io.IniFilename = nullptr;

    io.DisplaySize.x = width;
    io.DisplaySize.y = height;
    io.DeltaTime = 1.f / 60.f;

    // Set style
    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines = true;
    style.ChildRounding = 2.f;
    style.ChildBorderSize = 1.f;
    style.FrameRounding = 2.f;
    style.GrabRounding = 2.f;
    style.PopupBorderSize = 0.f;
    style.ScrollbarRounding = 2.f;
    style.ScrollbarSize = 12.f;
    style.WindowBorderSize = 1.f;
    style.WindowRounding = 4.f;
    style.Colors[ImGuiCol_Text] = ImVec4(0.89f, 0.91f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.11f, 0.15f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.01f, 0.02f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.02f, 0.02f, 0.04f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.01f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.01f, 0.03f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.01f, 0.01f, 0.01f, 0.65f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.01f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.02f, 0.02f, 0.04f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.02f, 0.04f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.01f, 0.03f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.06f, 0.28f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.06f, 0.28f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.11f, 0.34f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.06f, 0.28f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.25f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.05f, 0.31f, 0.96f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.05f, 0.31f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.01f, 0.13f, 0.53f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.01f, 0.13f, 0.53f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.05f, 0.31f, 0.96f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.05f, 0.31f, 0.96f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.05f, 0.31f, 0.96f, 0.95f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.01f, 0.02f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.05f, 0.31f, 0.96f, 0.80f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.01f, 0.02f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.01f, 0.02f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.16f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.79f, 0.46f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.33f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.05f, 0.31f, 0.96f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.05f, 0.31f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.61f, 0.61f, 0.61f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.61f, 0.61f, 0.61f, 0.35f);

    unsigned char* pixels;
    int fontWidth, fontHeight;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

    io.Fonts->TexID = _guiGfxImpl->initFontTexture(fontWidth, fontHeight, pixels);

    // Init clipboard callbacks
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.Platform_GetClipboardTextFn = Gui::getClipboardText;
    platformIO.Platform_SetClipboardTextFn = Gui::setClipboardText;
    platformIO.Platform_ClipboardUserData = static_cast<void*>(_renderingContext->getGLFWwindow());

    _isInitialized = true;
}

/*************/
const char* Gui::getClipboardText(ImGuiContext* userData)
{
    if (userData)
        return glfwGetClipboardString(reinterpret_cast<GLFWwindow*>(userData));
    else
        return nullptr;
}

/*************/
void Gui::setClipboardText(ImGuiContext* userData, const char* text)
{
    if (userData)
        glfwSetClipboardString(reinterpret_cast<GLFWwindow*>(userData), text);
}

/*************/
void Gui::initImWidgets()
{
    // Attributes can be shown in whichever widget,
    // this is why this widget is stored separately
    _guiAttributes = std::make_shared<GuiAttributes>(_scene, "Attributes");

    _guiWidgets.push_back(std::make_shared<GuiControl>(_scene, "Graph"));
    _guiWidgets.push_back(std::make_shared<GuiMeshes>(_scene, "Meshes"));
    _guiWidgets.push_back(std::make_shared<GuiMedia>(_scene, "Medias"));
    _guiWidgets.push_back(std::make_shared<GuiFilters>(_scene, "Filters"));

    auto globalView = std::make_shared<GuiCamera>(_scene, "Cameras");
    _guiWidgets.push_back(std::dynamic_pointer_cast<GuiWidget>(globalView));

    _guiWidgets.push_back(std::make_shared<GuiWarp>(_scene, "Warps"));

    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        _guiWidgets.push_back(std::make_shared<GuiGraph>(_scene, "Performances"));
        _guiWidgets.push_back(std::make_shared<GuiTexturesView>(_scene, "Textures"));
        _guiWidgets.push_back(std::make_shared<GuiTree>(_scene, "Tree view"));
    }

    // Bottom widgets
    // FPS and timings
    auto timingBox = std::make_shared<GuiTextBox>(_scene, "Timings");
    timingBox->setTextFunc([this]() {
        static std::unordered_map<std::string, float> stats;
        std::ostringstream stream;
        auto tree = _root->getTree();

        auto runningAverage = [](float a, float b) { return a * 0.9 + 0.001 * b * 0.1; };
        auto getLeafValue = [&](const std::string& path) {
            Value value;
            if (!tree->getValueForLeafAt(path, value))
                return 0.f;
            return value[0].as<float>();
        };

        // We process the world before the scenes
        {
            assert(tree->hasBranchAt("/world/durations"));

            stats["world_loop_world"] = runningAverage(stats["world_loop_world"], getLeafValue("/world/durations/loop_world"));
            stats["world_loop_world_fps"] = 1e3 / std::max(1.f, stats["world_loop_world"]);
            stats["world_upload"] = runningAverage(stats["world_upload"], getLeafValue("/world/durations/upload"));

            stream << "World:\n";
            stream << "  World framerate: " << std::setprecision(4) << stats["world_loop_world_fps"] << " fps\n";
            stream << "  Time per world frame: " << stats["world_loop_world"] << " ms\n";
            stream << "  Sending buffers to Scenes: " << std::setprecision(4) << stats["world_upload"] << " ms\n";
        }

        // Then the scenes
        stream << "Scenes:\n";
        for (const auto& branchName : tree->getBranchList())
        {
            if (branchName == "world")
                continue;
            auto durationPath = "/" + branchName + "/durations";

            stats[branchName + "_loop_scene"] = runningAverage(stats[branchName + "_loop_scene"], getLeafValue(durationPath + "/loop_scene"));
            stats[branchName + "_loop_scene_fps"] = 1e3 / std::max(1.f, stats[branchName + "_loop_scene"]);
            stats[branchName + "_textureUpload"] = runningAverage(stats[branchName + "_textureUpload"], getLeafValue(durationPath + "/textureUpload"));
            stats[branchName + "_blender"] = runningAverage(stats[branchName + "_blender"], getLeafValue(durationPath + "/blender"));
            stats[branchName + "_filter"] = runningAverage(stats[branchName + "_filter"], getLeafValue(durationPath + "/filter"));
            stats[branchName + "_camera"] = runningAverage(stats[branchName + "_camera"], getLeafValue(durationPath + "/camera"));
            stats[branchName + "_warp"] = runningAverage(stats[branchName + "_warp"], getLeafValue(durationPath + "/warp"));
            stats[branchName + "_renderingContext"] = runningAverage(stats[branchName + "_renderingContext"], getLeafValue(durationPath + "/window"));
            stats[branchName + "_swap"] = runningAverage(stats[branchName + "_swap"], getLeafValue(durationPath + "/swap"));
            stats[branchName + "_gl_time_per_frame"] =
                runningAverage(stats[branchName + "_gl_time_per_frame"], getLeafValue(durationPath + "/" + Constants::GL_TIMING_PREFIX + Constants::GL_TIMING_TIME_PER_FRAME));

            stream << "- " + branchName + ":\n";
            stream << "    GPU:\n";
            stream << "        Framerate: " << 1000.0 / stats[branchName + "_gl_time_per_frame"] << " fps\n";
            stream << "        Time per frame: " << stats[branchName + "_gl_time_per_frame"] << " ms\n";
            stream << "    CPU:\n";
            stream << "        Framerate: " << std::setprecision(4) << stats[branchName + "_loop_scene_fps"] << " fps\n";
            stream << "        Time per frame: " << stats[branchName + "_loop_scene"] << " ms\n";
            if (tree->hasLeafAt(durationPath + "/gui"))
            {
                stats[branchName + "_gui"] = runningAverage(stats[branchName + "_gui"], getLeafValue(durationPath + "/gui"));
                stream << "        GUI rendering: " << std::setprecision(4) << stats[branchName + "_gui"] << " ms\n";
            }
            stream << "        Texture upload: " << std::setprecision(4) << stats[branchName + "_textureUpload"] << " ms\n";
            stream << "        Blending: " << std::setprecision(4) << stats[branchName + "_blender"] << " ms\n";
            stream << "        Filters: " << std::setprecision(4) << stats[branchName + "_filter"] << " ms\n";
            stream << "        Cameras: " << std::setprecision(4) << stats[branchName + "_camera"] << " ms\n";
            stream << "        Warps: " << std::setprecision(4) << stats[branchName + "_warp"] << " ms\n";
            stream << "        Windows: " << std::setprecision(4) << stats[branchName + "_renderingContext"] << " ms\n";
            stream << "        Swapping: " << std::setprecision(4) << stats[branchName + "_swap"] << " ms\n";
        }

        return stream.str();
    });
    _guiBottomWidgets.push_back(std::dynamic_pointer_cast<GuiWidget>(timingBox));

    // Log display
    auto logBox = std::make_shared<GuiTextBox>(_scene, "Logs");
    logBox->setTextFunc([]() {
        int nbrLines = 10;
        // Convert the last lines of the text log
        std::vector<std::string> logs = Log::get().getLogs(Log::MESSAGE, Log::WARNING, Log::ERROR, Log::DEBUGGING);
        std::string text;
        uint32_t start = std::max(0, static_cast<int>(logs.size()) - nbrLines);
        for (uint32_t i = start; i < logs.size(); ++i)
            text += logs[i] + std::string("\n");

        return text;
    });
    _guiBottomWidgets.push_back(std::dynamic_pointer_cast<GuiWidget>(logBox));
}

/*************/
const char* Gui::getLocalKeyName(char key)
{
    return glfwGetKeyName(key, 0);
}

/*************/
void Gui::registerAttributes()
{
    ControllerObject::registerAttributes();

    addAttribute("size",
        [&](const Values& args) {
            setOutputSize(args[0].as<int>(), args[1].as<int>());
            return true;
        },
        {'i', 'i'});
    setAttributeDescription("size", "Set the GUI render resolution");

    addAttribute("hide",
        [&](const Values&) {
            _isVisible = false;
            return true;
        },
        {});
    setAttributeDescription("hide", "Hide the GUI");

    addAttribute("show",
        [&](const Values&) {
            _isVisible = true;
            return true;
        },
        {});
    setAttributeDescription("show", "Show the GUI");

    addAttribute(
        "fullscreen",
        [&](const Values& args) {
            _fullscreen = args[0].as<bool>();
            if (_fullscreen)
                _isVisible = true;
            return true;
        },
        [&]() -> Values { return {_fullscreen}; },
        {'b'});
    setAttributeDescription("fullscreen", "The GUI will take the whole window if set to true");

    addAttribute(
        "toggleDocking",
        [&](const Values& /*args*/) {
            toggleGuiDocking();
            return true;
        },
        [&]() -> Values { return {false}; },
        {'b'});
}

} // namespace Splash
