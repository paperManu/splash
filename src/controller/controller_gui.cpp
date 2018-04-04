#include "./controller/controller_gui.h"

#include <fstream>

#include "./controller/controller.h"
#include "./controller/widget/widget_control.h"
#include "./controller/widget/widget_filters.h"
#include "./controller/widget/widget_global_view.h"
#include "./controller/widget/widget_graph.h"
#include "./controller/widget/widget_media.h"
#include "./controller/widget/widget_meshes.h"
#include "./controller/widget/widget_node_view.h"
#include "./controller/widget/widget_template.h"
#include "./controller/widget/widget_text_box.h"
#include "./controller/widget/widget_textures_view.h"
#include "./controller/widget/widget_warp.h"
#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/window.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
GLuint Gui::_imFontTextureId;
GLuint Gui::_imGuiShaderHandle, Gui::_imGuiVertHandle, Gui::_imGuiFragHandle;
GLint Gui::_imGuiTextureLocation;
GLint Gui::_imGuiProjMatrixLocation;
GLint Gui::_imGuiPositionLocation;
GLint Gui::_imGuiUVLocation;
GLint Gui::_imGuiColorLocation;
GLuint Gui::_imGuiVboHandle, Gui::_imGuiElementsHandle, Gui::_imGuiVaoHandle;
size_t Gui::_imGuiVboMaxSize = 20000;

GLFWwindow* Gui::_glfwWindow = nullptr;

/*************/
Gui::Gui(shared_ptr<GlWindow> w, RootObject* s)
    : ControllerObject(s)
{
    _type = "gui";
    _renderingPriority = Priority::GUI;

    _scene = dynamic_cast<Scene*>(s);
    if (w.get() == nullptr || _scene == nullptr)
        return;

    _window = w;
    _glfwWindow = _window->get();
    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;
    glGetError();

    _fbo = make_unique<Framebuffer>(_root);
    _fbo->setResizable(true);

    _window->releaseContext();

    // Create the default GUI camera
    _guiCamera = make_shared<Camera>(s);
    _guiCamera->setName("guiCamera");
    _guiCamera->setAttribute("eye", {2.0, 2.0, 0.0});
    _guiCamera->setAttribute("target", {0.0, 0.0, 0.5});
    _guiCamera->setAttribute("size", {640, 480});

    // Intialize the GUI widgets
    _window->setAsCurrentContext();
    initImGui(_width, _height);
    initImWidgets();
    loadIcon();
    _window->releaseContext();

    registerAttributes();
}

/*************/
Gui::~Gui()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Gui::~Gui - Destructor" << Log::endl;
#endif

    glDeleteTextures(1, &_imFontTextureId);
    glDeleteProgram(_imGuiShaderHandle);
    glDeleteBuffers(1, &_imGuiVboHandle);
    glDeleteBuffers(1, &_imGuiElementsHandle);
    glDeleteVertexArrays(1, &_imGuiVaoHandle);
}

/*************/
void Gui::loadIcon()
{
    auto imagePath = string(DATADIR);
    string path{"splash-icon-512.png"};

    auto image = make_shared<Image>(_scene);
    image->setName("splash_icon");
    if (!image->read(imagePath + path))
    {
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - Could not find Splash icon, aborting image loading" << Log::endl;
        return;
    }

    _splashLogo = make_shared<Texture_Image>(_scene);
    _splashLogo->linkTo(image);
    _splashLogo->update();
    _splashLogo->flushPbo();
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
    string previousState = "none";
    if (!blendingState.empty())
        previousState = blendingState[0].as<string>();

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
void Gui::activateLUT()
{
    auto cameras = getObjectsOfType("camera");
    for (auto& cam : cameras)
    {
        setObjectAttribute(cam->getName(), "activateColorLUT", {2});
        setObjectAttribute(cam->getName(), "activateColorMixMatrix", {2});
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
void Gui::loadConfiguration()
{
    setWorldAttribute("loadConfig", {_configurationPath});
}

/*************/
void Gui::loadProject()
{
    setWorldAttribute("loadProject", {_projectPath});
}

/*************/
void Gui::copyCameraParameters()
{
    setWorldAttribute("copyCameraParameters", {_configurationPath});
}

/*************/
void Gui::saveConfiguration()
{
    setWorldAttribute("save", {_configurationPath});
}

/*************/
void Gui::saveProject()
{
    setWorldAttribute("saveProject", {_projectPath});
}

/*************/
void Gui::setJoystick(const vector<float>& axes, const vector<uint8_t>& buttons)
{
    for (auto& w : _guiWidgets)
        w->setJoystick(axes, buttons);
}

/*************/
void Gui::setJoystickState(const vector<UserInput::State>& state)
{
    vector<float> axes;
    vector<uint8_t> buttons;

    for (auto& s : state)
    {
        if (s.action == "joystick_0_axes")
            for (auto& v : s.value)
                axes.push_back(v.as<float>());
        else if (s.action == "joystick_0_buttons")
            for (auto& v : s.value)
                buttons.push_back(v.as<int>());
    }

    setJoystick(axes, buttons);
}

/*************/
void Gui::setKeyboardState(const vector<UserInput::State>& state)
{
    for (auto& s : state)
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
void Gui::setMouseState(const vector<UserInput::State>& state)
{
    if (!_window)
        return;

    for (auto& s : state)
    {
        auto parentIt = find_if(_parents.begin(), _parents.end(), [&](const BaseObject* p) { return s.window == p->getName(); });
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
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;
        io.KeyCtrl = ((mods & GLFW_MOD_CONTROL) != 0) && (action == GLFW_PRESS || action == GLFW_REPEAT);
        io.KeyShift = ((mods & GLFW_MOD_SHIFT) != 0) && (action == GLFW_PRESS || action == GLFW_REPEAT);

        break;
    }
    case GLFW_KEY_TAB:
    {
#if HAVE_OSX
        if (action == GLFW_PRESS && mods == GLFW_MOD_ALT)
#else
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
#endif
        {
            _isVisible = !_isVisible;
        }
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
#if HAVE_GPHOTO
    case GLFW_KEY_L:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
            activateLUT();
        break;
    }
    case GLFW_KEY_O:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
            calibrateColorResponseFunction();
        else
            goto sendAsDefault;
        break;
    }
    case GLFW_KEY_P:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
            calibrateColors();
        break;
    }
#endif
    case GLFW_KEY_S:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
            saveConfiguration();
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
    // Switch the rendering to textured
    case GLFW_KEY_T:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            _wireframe = false;
            setWorldAttribute("wireframe", {0});
        }
        break;
    }
    // Switch the rendering to wireframe
    case GLFW_KEY_W:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            _wireframe = true;
            setWorldAttribute("wireframe", {1});
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

    bool isPressed = action == GLFW_PRESS ? true : false;
    switch (btn)
    {
    default:
        break;
    case GLFW_MOUSE_BUTTON_LEFT:
        io.MouseDown[0] = isPressed;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        io.MouseDown[1] = isPressed;
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        io.MouseDown[2] = isPressed;
        break;
    }

    io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
    io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;

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
bool Gui::linkTo(const shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    if (!ControllerObject::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        auto object = dynamic_pointer_cast<Object>(obj);
        _guiCamera->linkTo(object);
        return true;
    }

    return false;
}

/*************/
void Gui::unlinkFrom(const shared_ptr<BaseObject>& obj)
{
    if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
        _guiCamera->unlinkFrom(obj);

    BaseObject::unlinkFrom(obj);
}

/*************/
void Gui::renderSplashScreen()
{
    static bool isOpen{false};
    static int splashWidth = 600, splashHeight = 288;
    ImGui::SetNextWindowPos(ImVec2((_width - splashWidth) / 2, (_height - splashHeight) / 2));
    ImGui::Begin("About Splash", &isOpen, ImVec2(splashWidth, splashHeight), 0.97f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    if (_splashLogo)
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::Image((void*)(intptr_t)_splashLogo->getTexId(), ImVec2(256, 256));
        ImGui::NextColumn();
        ImGui::Dummy(ImVec2(256, 80));
        ImGui::Text("Splash, a modular video-mapping engine");
        ImGui::Text("Version %s", PACKAGE_VERSION);
        ImGui::Spacing();
        ImGui::Text("Developed at the Société des Arts Technologiques");
        ImGui::Text("https://gitlab.com/sat-metalab/splash/wikis");
        ImGui::Columns(1);

        auto& io = ImGui::GetIO();
        if (io.MouseClicked[0])
            _showAbout = false;
    }
    ImGui::End();
}

/*************/
void Gui::render()
{
    if (!_isInitialized)
        return;

    ImageBufferSpec spec = _fbo->getColorTexture()->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);

    // If the gui is alone in its window, make it visible at startup
    if (_firstRender)
    {
        auto reversedLinks = getObjectReversedLinks();
        // If the GUI has multiple parents, hide it anyway
        if (reversedLinks[_name].size() == 1)
        {
            auto parent = reversedLinks[_name][0];
            auto allLinks = getObjectLinks();
            auto linksIt = allLinks.find(parent);
            if (linksIt != allLinks.end())
            {
                auto links = linksIt->second;
                bool hasOwnWindow = true;
                for (const auto& link : links)
                    if (link != _name)
                        hasOwnWindow = false;
                if (hasOwnWindow)
                    _isVisible = true;
            }
        }
    }

#ifdef DEBUG
    GLenum error = glGetError();
#endif
    using namespace ImGui;

    // Callback for dragndrop: load the dropped file
    UserInput::setCallback(UserInput::State("dragndrop"), [=](const UserInput::State& state) { setWorldAttribute("loadConfig", {state.value[0].as<string>()}); });

    ImGuiIO& io = GetIO();
    io.MouseDrawCursor = _mouseHoveringWindow;

    ImGui::NewFrame();

    // Panel
    if (_isVisible)
    {
        ImGui::Begin("Splash Control Panel", nullptr, ImVec2(700, 900), 0.95f, _windowFlags);
        _windowFlags = ImGuiWindowFlags_NoBringToFrontOnFocus;

        // Check whether the GUI is alone in its window
        auto objReversedLinks = getObjectReversedLinks();
        auto objLinks = getObjectLinks();
        if (objLinks[objReversedLinks[_name][0]].size() == 1)
        {
            ImGui::SetWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);
            ImGui::SetWindowSize(ImVec2(_width, _height));
            _windowFlags |= ImGuiWindowFlags_NoMove;
        }
        else
        {
            ImGui::SetWindowPos(ImVec2(_initialGuiPos[0], _initialGuiPos[1]), ImGuiSetCond_Once);
        }

        // About
        if (ImGui::Button("About Splash", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
            _showAbout = true;

        // Some global buttons
        if (ImGui::CollapsingHeader("General commands", nullptr, true, true))
        {
#if HAVE_GPHOTO
            ImGui::Columns(2);
            ImGui::Text("General");
            ImGui::NextColumn();
            ImGui::Text("Color calibration");
            ImGui::NextColumn();
            ImGui::Separator();
#endif
            if (ImGui::Button("Compute blending map"))
                computeBlending(true);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("To use once cameras are calibrated (Ctrl+B, Ctrl+Alt+B to compute at each frames)");

            if (ImGui::Button("Flash background"))
                setObjectsOfType("camera", "flashBG", {});
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Set the background as light gray (Ctrl+F)");

            if (ImGui::Button("Wireframe / Textured"))
            {
                _wireframe = !_wireframe;
                setWorldAttribute("wireframe", {(int)_wireframe});
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Switch objects between wireframe and textured (Ctrl+T and Ctrl+W)");

#if HAVE_GPHOTO
            ImGui::NextColumn();
            if (ImGui::Button("Calibrate camera response"))
                calibrateColorResponseFunction();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Needs to be done before projector calibration (Ctrl+O)");

            if (ImGui::Button("Calibrate displays / projectors"))
                calibrateColors();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Calibrates all outputs (Ctrl+P)");

            if (ImGui::Button("Activate correction"))
                activateLUT();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Activate color LUT, once calibrated (Ctrl+L)");

            ImGui::Columns(1);
#endif
            // Configuration load
            ImGui::Separator();
            ImGui::Text("Configuration file");
            char configurationPath[512];
            strncpy(configurationPath, _configurationPath.data(), 511);
            ImGui::InputText("##ConfigurationPath", configurationPath, 512);
            _configurationPath = string(configurationPath);

            ImGui::SameLine();
            static bool showConfigurationFileSelector{false};
            if (ImGui::Button("...##Configuration"))
                showConfigurationFileSelector = true;
            if (showConfigurationFileSelector)
            {
                static string path = _root->getConfigurationPath();
                bool cancelled;
                if (SplashImGui::FileSelector("Configuration", path, cancelled, {{"json"}}))
                {
                    if (!cancelled)
                        _configurationPath = Utils::isDir(path) ? path + "configuration.json" : path;
                    else
                        path = _root->getConfigurationPath();
                    showConfigurationFileSelector = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Save##Configuration"))
                saveConfiguration();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Configuration is saved to the given path (Ctrl+S)");

            ImGui::SameLine();
            if (ImGui::Button("Load##Configuration"))
                loadConfiguration();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Load the given path");

            ImGui::SameLine();
            if (ImGui::Button("Copy"))
                copyCameraParameters();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Copy the camera parameters from the given file");

            // Project load
            ImGui::Separator();
            ImGui::Text("Project file");
            char projectPath[512];
            strncpy(projectPath, _projectPath.data(), 511);
            ImGui::InputText("##ProjectPath", projectPath, 512);
            _projectPath = string(projectPath);

            ImGui::SameLine();
            static bool showProjectFileSelector{false};
            if (ImGui::Button("...##Project"))
                showProjectFileSelector = true;
            if (showProjectFileSelector)
            {
                static string path = _root->getConfigurationPath();
                bool cancelled;
                if (SplashImGui::FileSelector("Project", path, cancelled, {{"json"}}))
                {
                    if (!cancelled)
                        _projectPath = Utils::isDir(path) ? path + "project.json" : path;
                    else
                        path = _root->getConfigurationPath();
                    showProjectFileSelector = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Save##Project"))
                saveProject();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save a project configuration (only media and meshes)");

            ImGui::SameLine();
            if (ImGui::Button("Load##Project"))
                loadProject();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Load a project configuration (only media and meshes)");

            // Media directory
            ImGui::Separator();
            ImGui::Text("Media directory");
            char mediaPath[512];
            strncpy(mediaPath, _root->getMediaPath().data(), 511);
            if (ImGui::InputText("##MediaPath", mediaPath, 512))
                setWorldAttribute("mediaPath", {string(mediaPath)});

            ImGui::SameLine();
            static bool showMediaFileSelector{false};
            if (ImGui::Button("...##Media"))
                showMediaFileSelector = true;
            if (showMediaFileSelector)
            {
                static string path = _root->getMediaPath();
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
        }

        // Specific widgets
        for (auto& widget : _guiWidgets)
        {
            widget->render();
            _windowFlags |= widget->updateWindowFlags();
        }
        ImGui::End();
    }

    if (_showAbout)
        renderSplashScreen();

    // Uncomment to show styling gui!
    // ImGuiStyle& style = ImGui::GetStyle();
    // ImGui::ShowStyleEditor(&style);

    static double time = 0.0;
    const double currentTime = glfwGetTime();
    io.DeltaTime = static_cast<float>(currentTime - time);
    time = currentTime;

    if (_isVisible || _showAbout || _wasVisible || _resized)
    {
        _wasVisible = _isVisible;

        _fbo->bindDraw();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        if (_isVisible || _showAbout)
            ImGui::Render();
        _fbo->unbindDraw();
    }

    _fbo->getColorTexture()->generateMipmap();

#ifdef DEBUG
    error = glGetError();
    if (error)
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while rendering the gui: " << error << Log::endl;
#endif

    _firstRender = false;
    _resized = false;

    return;
}

/*************/
void Gui::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;

    _fbo->setSize(width, height);

    _width = width;
    _height = height;
    _resized = true;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;

    _window->releaseContext();
}

/*************/
void Gui::initImGui(int width, int height)
{
    using namespace ImGui;

    // Initialize GL stuff for ImGui
    const std::string vertexShader{R"(
        #version 330 core

        uniform mat4 ProjMtx;
        in vec2 Position;
        in vec2 UV;
        in vec4 Color;
        out vec2 Frag_UV;
        out vec4 Frag_Color;

        void main()
        {
            Frag_UV = UV;
            Frag_Color = Color;
            //Frag_Color.a += 0.5f;
            gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
        }
    )"};

    const std::string fragmentShader{R"(
        #version 330 core

        uniform sampler2D Texture;
        in vec2 Frag_UV;
        in vec4 Frag_Color;
        out vec4 Out_Color;

        void main()
        {
            Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
        }
    )"};

    _imGuiShaderHandle = glCreateProgram();
    _imGuiVertHandle = glCreateShader(GL_VERTEX_SHADER);
    _imGuiFragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    {
        const char* shaderSrc = vertexShader.c_str();
        glShaderSource(_imGuiVertHandle, 1, (const GLchar**)&shaderSrc, 0);
    }
    {
        const char* shaderSrc = fragmentShader.c_str();
        glShaderSource(_imGuiFragHandle, 1, (const GLchar**)&shaderSrc, 0);
    }
    glCompileShader(_imGuiVertHandle);
    glCompileShader(_imGuiFragHandle);
    glAttachShader(_imGuiShaderHandle, _imGuiVertHandle);
    glAttachShader(_imGuiShaderHandle, _imGuiFragHandle);
    glLinkProgram(_imGuiShaderHandle);

    GLint status;
    glGetProgramiv(_imGuiShaderHandle, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        Log::get() << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error while linking the shader program" << Log::endl;

        GLint length;
        glGetProgramiv(_imGuiShaderHandle, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*)malloc(length);
        glGetProgramInfoLog(_imGuiShaderHandle, length, &length, log);
        Log::get() << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error log: \n" << (const char*)log << Log::endl;
        free(log);

        // TODO: handle this case...
        return;
    }

    _imGuiTextureLocation = glGetUniformLocation(_imGuiShaderHandle, "Texture");
    _imGuiProjMatrixLocation = glGetUniformLocation(_imGuiShaderHandle, "ProjMtx");
    _imGuiPositionLocation = glGetAttribLocation(_imGuiShaderHandle, "Position");
    _imGuiUVLocation = glGetAttribLocation(_imGuiShaderHandle, "UV");
    _imGuiColorLocation = glGetAttribLocation(_imGuiShaderHandle, "Color");

    glCreateBuffers(1, &_imGuiVboHandle);
    glCreateBuffers(1, &_imGuiElementsHandle);

    glCreateVertexArrays(1, &_imGuiVaoHandle);
    glBindVertexArray(_imGuiVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
    glEnableVertexAttribArray(_imGuiPositionLocation);
    glEnableVertexAttribArray(_imGuiUVLocation);
    glEnableVertexAttribArray(_imGuiColorLocation);

    glVertexAttribPointer(_imGuiPositionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)(size_t) & (((ImDrawVert*)0)->pos));
    glVertexAttribPointer(_imGuiUVLocation, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)(size_t) & (((ImDrawVert*)0)->uv));
    glVertexAttribPointer(_imGuiColorLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)(size_t) & (((ImDrawVert*)0)->col));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Initialize ImGui
    ImGuiIO& io = GetIO();

    string fontPath = "";
    vector<string> fontPaths{string(DATADIR) + string("fonts/Roboto-Medium.ttf"), "../Resources/fonts/OpenSans-Bold.ttf"};
    for (auto& path : fontPaths)
        if (ifstream(path, ios::in | ios::binary))
            fontPath = path;

    if (fontPath != "")
    {
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15);
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15);
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15);
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15);
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15);
        io.Fonts->Build();
    }

    io.IniFilename = nullptr;

    io.DisplaySize.x = width;
    io.DisplaySize.y = height;
    io.DeltaTime = 1.f / 60.f;

    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;

    io.RenderDrawListsFn = Gui::imGuiRenderDrawLists;

    // Set style
    ImGuiStyle& style = ImGui::GetStyle();
    style.ChildWindowRounding = 2.f;
    style.FrameRounding = 2.f;
    style.ScrollbarSize = 12.f;
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.45f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.98f, 0.58f, 0.12, 0.74f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.98f, 0.58f, 0.12, 0.74f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.98f, 0.58f, 0.12, 0.74f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.58f, 0.12, 0.74f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.81f, 0.40f, 0.25f, 0.27f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.81f, 0.40f, 0.24f, 0.40f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.80f, 0.50f, 0.50f, 0.40f);
    style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
    style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.60f, 0.40f, 0.40f, 0.45f);
    style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.65f, 0.50f, 0.50f, 0.55f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.67f, 0.40f, 0.40f, 0.60f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.81f, 0.40f, 0.24f, 0.45f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.79f, 0.45f, 0.17f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.79f, 0.53f, 0.21f, 0.80f);
    style.Colors[ImGuiCol_Column] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.60f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.80f, 0.47f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_CloseButton] = ImVec4(0.81f, 0.50f, 0.28f, 0.50f);
    style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
    style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
    style.AntiAliasedLines = false;

    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

    // Set GL texture for font
    glDeleteTextures(1, &_imFontTextureId);
    glGenTextures(1, &_imFontTextureId);
    glBindTexture(GL_TEXTURE_2D, _imFontTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    io.Fonts->TexID = (void*)(intptr_t)_imFontTextureId;

    // Init clipboard callbacks
    io.GetClipboardTextFn = Gui::getClipboardText;
    io.SetClipboardTextFn = Gui::setClipboardText;

    _isInitialized = true;
}

/*************/
const char* Gui::getClipboardText(void* /*userData*/)
{
    if (_glfwWindow)
        return glfwGetClipboardString(_glfwWindow);
    else
        return nullptr;
}

/*************/
void Gui::setClipboardText(void* /*userData*/, const char* text)
{
    if (_glfwWindow)
        glfwSetClipboardString(_glfwWindow, text);
}

/*************/
void Gui::initImWidgets()
{
    // Template configurations
    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        auto templateBox = make_shared<GuiTemplate>(_scene, "Templates");
        templateBox->setScene(_scene);
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(templateBox));
    }

    // Some help regarding keyboard shortcuts
    auto helpBox = make_shared<GuiTextBox>(_scene, "Shortcuts");
    helpBox->setTextFunc([]() {
        string text = R"(Tab: show / hide this GUI
        General shortcuts:
         Ctrl+S: save the current configuration
         Ctrl+F: white background instead of black
         Ctrl+B: compute the blending between all projectors
         Ctrl+Alt+B: compute the blending between all projectors at every frame
         Ctrl+M: hide/show the OS cursor
         Ctrl+T: textured draw mode
         Ctrl+W: wireframe draw mode
        )";
#if HAVE_GPHOTO
        text += R"(
         Ctrl+O: launch camera color calibration
         Ctrl+P: launch projectors color calibration
         Ctrl+L: activate color LUT (if calibrated)
        )";
#endif
        text += R"(
        Views panel:
         Ctrl + left click on a camera thumbnail: hide / show the given camera
         Space: switch between projectors
         A: show / hide the target calibration point
         C: calibrate the selected camera
         H: hide all but the selected camera
         O: show calibration points from all cameras
         Ctrl+Z: revert camera to previous calibration

        Node view (inside Control panel):
         Shift + left click: link the clicked node to the selected one
         Ctrl + left click: unlink the clicked node from the selected one

        Joystick controls (may vary with the controller):
         Directions: move the selected calibration point
         Button 1: select previous calibration point
         Button 2: select next calibration point
         Button 3: hide all but the selected cameras
         Button 4: calibrate
         Button 5: move calibration point slower
         Button 6: move calibration point faster
         Button 7: flash the background to light gray
        )";

        return text;
    });
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(helpBox));

    // FPS and timings
    auto timingBox = make_shared<GuiTextBox>(_scene, "Timings");
    timingBox->setTextFunc([]() {
        // Smooth the values
        static float sce{0.f};
        static float wrl{0.f};
        static float fps{0.f};
        static float worldFps{0.f};
        static float upl{0.f};
        static float tex{0.f};
        static float ble{0.f};
        static float flt{0.f};
        static float cam{0.f};
        static float wrp{0.f};
        static float gui{0.f};
        static float win{0.f};
        static float buf{0.f};
        static float evt{0.f};

        sce = sce * 0.9 + Timer::get()["loop_scene"] * 0.001 * 0.1;
        wrl = wrl * 0.9 + Timer::get()["loop_world"] * 0.001 * 0.1;
        fps = 1e3 / std::max(1.f, sce);
        worldFps = 1e3 / std::max(1.f, wrl);
        upl = upl * 0.9 + Timer::get()["upload"] * 0.001 * 0.1;
        tex = tex * 0.9 + Timer::get()["textureUpload"] * 0.001 * 0.1;
        ble = ble * 0.9 + Timer::get()["blender"] * 0.001 * 0.1;
        flt = flt * 0.9 + Timer::get()["filter"] * 0.001 * 0.1;
        cam = cam * 0.9 + Timer::get()["camera"] * 0.001 * 0.1;
        wrp = wrp * 0.9 + Timer::get()["warp"] * 0.001 * 0.1;
        gui = gui * 0.9 + Timer::get()["gui"] * 0.001 * 0.1;
        win = win * 0.9 + Timer::get()["window"] * 0.001 * 0.1;
        buf = buf * 0.9 + Timer::get()["swap"] * 0.001 * 0.1;
        evt = evt * 0.9 + Timer::get()["events"] * 0.001 * 0.1;

        // Create the text message
        ostringstream stream;
        Timer::Point clock;
        if (Timer::get().getMasterClock(clock))
        {
            stream << "Master clock: " << clock.years << "/" << clock.months << "/" << clock.days << " - " << clock.hours << ":" << clock.mins << ":" << clock.secs << ":"
                   << clock.frame;
            if (clock.paused)
                stream << " - Paused";
            stream << "\n";
        }
        stream << "World:\n";
        stream << "  World framerate: " << setprecision(4) << worldFps << " fps\n";
        stream << "  Time per world frame: " << wrl << " ms\n";
        stream << "  Sending buffers to Scenes: " << setprecision(4) << upl << " ms\n";
        stream << "Rendering:\n";
        stream << "  Rendering framerate: " << setprecision(4) << fps << " fps\n";
        stream << "  Time per rendered frame: " << sce << " ms\n";
        stream << "  Texture upload: " << setprecision(4) << tex << " ms\n";
        stream << "  Blending computation: " << setprecision(4) << ble << " ms\n";
        stream << "  Filters: " << setprecision(4) << flt << " ms\n";
        stream << "  Cameras rendering: " << setprecision(4) << cam << " ms\n";
        stream << "  Warps: " << setprecision(4) << wrp << " ms\n";
        stream << "  GUI rendering: " << setprecision(4) << gui << " ms\n";
        stream << "  Windows rendering: " << setprecision(4) << win << " ms\n";
        stream << "  Swapping and events: " << setprecision(4) << buf << " ms\n";

        return stream.str();
    });
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(timingBox));

    // Log display
    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        auto logBox = make_shared<GuiTextBox>(_scene, "Logs");
        logBox->setTextFunc([]() {
            int nbrLines = 10;
            // Convert the last lines of the text log
            vector<string> logs = Log::get().getLogs(Log::MESSAGE, Log::WARNING, Log::ERROR, Log::DEBUGGING);
            string text;
            uint32_t start = std::max(0, static_cast<int>(logs.size()) - nbrLines);
            for (uint32_t i = start; i < logs.size(); ++i)
                text += logs[i] + string("\n");

            return text;
        });
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(logBox));
    }

    // Control
    auto controlView = make_shared<GuiControl>(_scene, "Controls");
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(controlView));

    // Media
    auto mediaSelector = make_shared<GuiMedia>(_scene, "Media");
    _guiWidgets.push_back(mediaSelector);

    // Filters
    auto filterPanel = make_shared<GuiFilters>(_scene, "Filters");
    _guiWidgets.push_back(filterPanel);

    // Meshes
    auto meshesSelector = make_shared<GuiMeshes>(_scene, "Meshes");
    _guiWidgets.push_back(meshesSelector);

    // GUI camera view
    auto globalView = make_shared<GuiGlobalView>(_scene, "Cameras");
    globalView->setCamera(_guiCamera);
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(globalView));

    // Warp control
    auto warpControl = make_shared<GuiWarp>(_scene, "Warp");
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWarp>(warpControl));

    // Performance graph
    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        auto perfGraph = make_shared<GuiGraph>(_scene, "Performance Graph");
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(perfGraph));

        auto texturesView = make_shared<GuiTexturesView>(_scene, "Textures");
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(texturesView));
    }
}

/*************/
void Gui::imGuiRenderDrawLists(ImDrawData* draw_data)
{
    if (!draw_data->CmdListsCount)
        return;

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float orthoProjection[4][4] = {
        {2.0f / width, 0.0f, 0.0f, 0.0f}, {0.0f, 2.0f / -height, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f, 1.0f},
    };

    glUseProgram(_imGuiShaderHandle);
    glUniform1i(_imGuiTextureLocation, 0);
    glUniformMatrix4fv(_imGuiProjMatrixLocation, 1, GL_FALSE, (float*)orthoProjection);
    glBindVertexArray(_imGuiVaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _imGuiElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); ++pcmd)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
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
        {'n', 'n'});
    setAttributeDescription("size", "Set the GUI render resolution");
}

} // end of namespace
