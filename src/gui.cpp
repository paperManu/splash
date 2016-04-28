#include "gui.h"

#include "camera.h"
#include "log.h"
#include "object.h"
#include "osUtils.h"
#include "scene.h"
#include "texture.h"
#include "texture_image.h"
#include "timer.h"
#include "threadpool.h"
#include "window.h"

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

/*************/
Gui::Gui(GlWindowPtr w, SceneWeakPtr s)
{
    _type = "gui";

    auto scene = s.lock();
    if (w.get() == nullptr || scene.get() == nullptr)
        return;

    _scene = s;
    _window = w;
    if (!_window->setAsCurrentContext()) 
    	 Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    glGetError();
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    {
        Texture_ImagePtr texture = make_shared<Texture_Image>();
        texture->reset(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        texture->setResizable(1);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getTexId(), 0);
        _depthTexture = move(texture);
    }

    {
        Texture_ImagePtr texture = make_shared<Texture_Image>();
        texture->reset(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
        texture->setResizable(1);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getTexId(), 0);
        _outTexture = move(texture);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while initializing framebuffer object: " << status << Log::endl;
    else
        Log::get() << Log::MESSAGE << "Gui::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    _window->releaseContext();

    // Create the default GUI camera
    scene->_mainWindow->setAsCurrentContext();
    _guiCamera = make_shared<Camera>(s);
    _guiCamera->setName("guiCamera");
    _guiCamera->setAttribute("eye", {2.0, 2.0, 0.0});
    _guiCamera->setAttribute("target", {0.0, 0.0, 0.5});
    _guiCamera->setAttribute("size", {640, 480});
    scene->_mainWindow->releaseContext();

    // Intialize the GUI widgets
    _window->setAsCurrentContext();
    initImGui(_width, _height);
    initImWidgets();
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
    auto scene = _scene.lock();
    scene->sendMessageToWorld("computeBlending", {(int)once});
}

/*************/
void Gui::activateLUT()
{
    auto scene = _scene.lock();
    vector<CameraPtr> cameras;
    for (auto& obj : scene->_objects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& obj : scene->_ghostObjects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& cam : cameras)
    {
        scene->sendMessageToWorld("sendAll", {cam->getName(), "activateColorLUT", 2});
        scene->sendMessageToWorld("sendAll", {cam->getName(), "activateColorMixMatrix", 2});
    }
}

/*************/
void Gui::calibrateColorResponseFunction()
{
    auto scene = _scene.lock();
    scene->setAttribute("calibrateColorResponseFunction", {});
}

/*************/
void Gui::calibrateColors()
{
    auto scene = _scene.lock();
    scene->setAttribute("calibrateColor", {});
}

/*************/
void Gui::loadConfiguration()
{
    auto scene = _scene.lock();
    scene->sendMessageToWorld("loadConfig", {_configurationPath});
}

/*************/
void Gui::saveConfiguration()
{
    auto scene = _scene.lock();
    scene->sendMessageToWorld("save", {_configurationPath});
}

/*************/
void Gui::flashBackground()
{
    auto scene = _scene.lock();
    if (_flashBG)
        scene->sendMessageToWorld("flashBG", {0});
    else
        scene->sendMessageToWorld("flashBG", {1});
    _flashBG = !_flashBG;
}

/*************/
void Gui::setJoystick(const vector<float>& axes, const vector<uint8_t>& buttons)
{
    for (auto& w : _guiWidgets)
        w->setJoystick(axes, buttons);
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
        if (action == GLFW_PRESS)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;
        io.KeyCtrl = ((mods & GLFW_MOD_CONTROL) != 0) && (action == GLFW_PRESS);
        io.KeyShift = ((mods & GLFW_MOD_SHIFT) != 0) && (action == GLFW_PRESS);
    
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
    case GLFW_KEY_ESCAPE:
    {
        if (action == GLFW_PRESS)
        {
            auto scene = _scene.lock();
            scene->sendMessageToWorld("quit");
        }
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
            flashBackground();
        break;
    }
    case GLFW_KEY_M:
    {
        if (mods == GLFW_MOD_CONTROL && action == GLFW_PRESS)
        {
            static bool cursorVisible = false;
            cursorVisible = !cursorVisible;

            auto scene = _scene.lock();
            for (auto& obj : scene->_objects)
                if (obj.second->getType() == "window")
                    dynamic_pointer_cast<Window>(obj.second)->showCursor(cursorVisible);
        }
        break;
    }
    // Switch the rendering to textured
    case GLFW_KEY_T: 
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            auto scene = _scene.lock();
            _wireframe = false;
            scene->sendMessageToWorld("wireframe", {0});
        }
        break;
    }
    // Switch the rendering to wireframe
    case GLFW_KEY_W:
    {
        if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
        {
            auto scene = _scene.lock();
            _wireframe = true;
            scene->sendMessageToWorld("wireframe", {1});
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

    int button {0};
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
void Gui::mouseScroll(double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += (float)yoffset;

    return;
}

/*************/
bool Gui::linkTo(shared_ptr<BaseObject> obj)
{
    // Mandatory before trying to link
    if (!BaseObject::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        ObjectPtr object = dynamic_pointer_cast<Object>(obj);
        _guiCamera->linkTo(object);
        return true;
    }

    return false;
}

/*************/
bool Gui::unlinkFrom(shared_ptr<BaseObject> obj)
{
    if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        _guiCamera->unlinkFrom(obj);
    }

    return BaseObject::unlinkFrom(obj);
}

/*************/
bool Gui::render()
{
    if (!_isInitialized)
        return false;

    ImageBufferSpec spec = _outTexture->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);

#ifdef DEBUG
    GLenum error = glGetError();
#endif

    if (_isVisible)
    {
        using namespace ImGui;

        ImGuiIO& io = GetIO();
        io.MouseDrawCursor = true;

        ImGui::NewFrame();

        ImGui::Begin("Splash Control Panel", nullptr, ImVec2(700, 900), 0.95f, _windowFlags);
        ImGui::SetWindowPos(ImVec2(16, 16), ImGuiSetCond_Once);
        _windowFlags = 0;

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
                flashBackground();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Set the background as light gray (Ctrl+F)");

            if (ImGui::Button("Wireframe / Textured"))
            {
                auto scene = _scene.lock();
                _wireframe = !_wireframe;
                scene->sendMessageToWorld("wireframe", {(int)_wireframe});
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
            ImGui::Separator();
            ImGui::Text("Configuration file");
            char configurationPath[512];
            strcpy(configurationPath, _configurationPath.data());
            ImGui::InputText("##Path", configurationPath, 512);
            _configurationPath = string(configurationPath);

            ImGui::SameLine();
            static bool showFileSelector {false};
            if (ImGui::Button("..."))
            {
                showFileSelector = true;
            }
            if (showFileSelector)
            {
                static string path = Utils::getPathFromFilePath("./");
                bool cancelled;
                if (SplashImGui::FileSelector("Configuration", path, cancelled, {{"json"}}))
                {
                    if (!cancelled)
                    {
                        _configurationPath = path;
                        path = Utils::getPathFromFilePath("./");
                    }
                    showFileSelector = false;
                }
            }

            if (ImGui::Button("Save configuration"))
                saveConfiguration();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Configuration is saved to the given path (Ctrl+S)");

            ImGui::SameLine();
            if (ImGui::Button("Load configuration"))
                loadConfiguration();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Load the given path");
        }

        // Specific widgets
        for (auto& widget : _guiWidgets)
        {
            widget->render();
            _windowFlags |= widget->updateWindowFlags();
        }
        ImGui::End();

        // Uncomment to show styling gui!
        //ImGuiStyle& style = ImGui::GetStyle();
        //ImGui::ShowStyleEditor(&style);

        static double time = 0.0;
        const double currentTime = glfwGetTime();
        io.DeltaTime = (float)(currentTime - time);
        time = currentTime;

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, fboBuffers);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
    }
    else
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, fboBuffers);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    auto outTexture_asImage = dynamic_pointer_cast<Texture_Image>(_outTexture);
    if (outTexture_asImage)
        outTexture_asImage->generateMipmap();

#ifdef DEBUG
    error = glGetError();
    if (error)
        Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while rendering the gui: " << error << Log::endl;

    return error != 0 ? true : false;
#else
    return false;
#endif
}

/*************/
void Gui::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    if (!_window->setAsCurrentContext()) 
    	 Log::get() << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    _depthTexture->setAttribute("size", {width, height});
    _outTexture->setAttribute("size", {width, height});

    _width = width;
    _height = height;

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
    const std::string vertexShader {R"(
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

    const std::string fragmentShader {R"(
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

    glGenBuffers(1, &_imGuiVboHandle);
    glGenBuffers(1, &_imGuiElementsHandle);
    //glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
    //glBufferData(GL_ARRAY_BUFFER, _imGuiVboMaxSize, NULL, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &_imGuiVaoHandle);
    glBindVertexArray(_imGuiVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
    glEnableVertexAttribArray(_imGuiPositionLocation);
    glEnableVertexAttribArray(_imGuiUVLocation);
    glEnableVertexAttribArray(_imGuiColorLocation);

    glVertexAttribPointer(_imGuiPositionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)(size_t)&(((ImDrawVert*)0)->pos));
    glVertexAttribPointer(_imGuiUVLocation, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)(size_t)&(((ImDrawVert*)0)->uv));
    glVertexAttribPointer(_imGuiColorLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)(size_t)&(((ImDrawVert*)0)->col));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Initialize ImGui
    ImGuiIO& io = GetIO();

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
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = Gui::imGuiRenderDrawLists;

    // Set style
    ImGuiStyle& style = ImGui::GetStyle();
    style.ChildWindowRounding = 2.f;
    style.FrameRounding = 2.f;
    style.ScrollbarSize = 12.f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.80f, 0.80f, 0.80f, 0.45f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(1.00f, 0.50f, 0.25f, 0.74f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.81f, 0.40f, 0.25f, 0.45f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(1.00f, 0.50f, 0.25f, 0.74f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.79f, 0.40f, 0.25f, 0.15f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.81f, 0.40f, 0.25f, 0.27f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.81f, 0.40f, 0.24f, 0.40f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.80f, 0.50f, 0.50f, 0.40f);
    style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.60f, 0.40f, 0.40f, 0.45f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.65f, 0.50f, 0.50f, 0.55f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.67f, 0.40f, 0.40f, 0.60f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.67f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.81f, 0.40f, 0.24f, 0.45f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.79f, 0.45f, 0.17f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.79f, 0.53f, 0.21f, 0.80f);
    style.Colors[ImGuiCol_Column]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.60f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.80f, 0.47f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.81f, 0.50f, 0.28f, 0.50f);
    style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
    style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_TooltipBg]             = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
    style.AntiAliasedLines = false;

    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

    glDeleteTextures(1, &_imFontTextureId);
    glGenTextures(1, &_imFontTextureId);
    glBindTexture(GL_TEXTURE_2D, _imFontTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    io.Fonts->TexID = (void*)(intptr_t)_imFontTextureId;

    _isInitialized = true;
}

/*************/
void Gui::initImWidgets()
{
    // Template configurations
    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        auto templateBox = make_shared<GuiTemplate>("Templates");
        templateBox->setScene(_scene);
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(templateBox));
    }

    // Some help regarding keyboard shortcuts
    auto helpBox = make_shared<GuiTextBox>("Shortcuts");
    helpBox->setTextFunc([]()
    {
        string text;
        text += "Tab: show / hide this GUI\n";
        text += "General shortcuts:\n";
        text += " Ctrl+F: white background instead of black\n";
        text += " Ctrl+B: compute the blending between all projectors\n";
        text += " Ctrl+Alt+B: compute the blending between all projectors at every frame\n";
        text += " Ctrl+M: hide/show the OS cursor\n";
        text += " Ctrl+T: textured draw mode\n";
        text += " Ctrl+W: wireframe draw mode\n";
#if HAVE_GPHOTO
        text += "\n";
        text += " Ctrl+O: launch camera color calibration\n";
        text += " Ctrl+P: launch projectors color calibration\n";
        text += " Ctrl+L: activate color LUT (if calibrated)\n";
#endif
        text += "\n";
        text += "Views panel:\n";
        text += " Ctrl + left click on a camera thumbnail: hide / show the given camera\n";
        text += " Space: switch between projectors\n";
        text += " A: show / hide the target calibration point\n";
        text += " C: calibrate the selected camera\n";
        text += " H: hide all but the selected camera\n";
        text += " O: show calibration points from all cameras\n";
        text += " Ctrl+Z: revert camera to previous calibration\n";

        text += "\n";
        text += "Node view (inside Control panel):\n";
        text += " Shift + left click: link the clicked node to the selected one\n";
        text += " Ctrl + left click: unlink the clicked node from the selected one\n";

        return text;
    });
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(helpBox));

    // FPS and timings
    auto timingBox = make_shared<GuiTextBox>("Timings");
    timingBox->setTextFunc([]()
    {
        // Smooth the values
        static float fps {0.f};
        static float worldFps {0.f};
        static float upl {0.f};
        static float tex {0.f};
        static float ble {0.f};
        static float flt {0.f};
        static float cam {0.f};
        static float wrp {0.f};
        static float gui {0.f};
        static float win {0.f};
        static float buf {0.f};
        static float evt {0.f};

        fps = fps * 0.95 + 1e6 / std::max(1ull, Timer::get()["sceneLoop"]) * 0.05;
        worldFps = worldFps * 0.9 + 1e6 / std::max(1ull, Timer::get()["worldLoop"]) * 0.1;
        upl = upl * 0.9 + Timer::get()["upload"] * 0.001 * 0.1;
        tex = tex * 0.9 + Timer::get()["textureUpload"] * 0.001 * 0.1;
        ble = ble * 0.9 + Timer::get()["blending"] * 0.001 * 0.1;
        flt = flt * 0.9 + Timer::get()["filters"] * 0.001 * 0.1;
        cam = cam * 0.9 + Timer::get()["cameras"] * 0.001 * 0.1;
        wrp = wrp * 0.9 + Timer::get()["warps"] * 0.001 * 0.1;
        gui = gui * 0.9 + Timer::get()["gui"] * 0.001 * 0.1;
        win = win * 0.9 + Timer::get()["windows"] * 0.001 * 0.1;
        buf = buf * 0.9 + Timer::get()["swap"] * 0.001 * 0.1;
        evt = evt * 0.9 + Timer::get()["events"] * 0.001 * 0.1;


        // Create the text message
        ostringstream stream;
        Values clock;
        if (Timer::get().getMasterClock(clock))
        {
            stream << "Master clock: " << clock[0].asInt() << "/" << clock[1].asInt() << "/" << clock[2].asInt() << " - " << clock[3].asInt() << ":" << clock[4].asInt() << ":" << clock[5].asInt() << ":" << clock[6].asInt();
            if (clock[7].asInt() == 1)
                stream << " - Paused";
            stream << "\n";
        }
        stream << "Framerate: " << setprecision(4) << fps << " fps\n";
        stream << "World framerate: " << setprecision(4) << worldFps << " fps\n";
        stream << "Sending buffers to Scenes: " << setprecision(4) << upl << " ms\n";
        stream << "Texture upload: " << setprecision(4) << tex << " ms\n";
        stream << "Blending computation: " << setprecision(4) << ble << " ms\n";
        stream << "Filters: " << setprecision(4) << flt << " ms\n";
        stream << "Cameras rendering: " << setprecision(4) << cam << " ms\n";
        stream << "Warps: " << setprecision(4) << wrp << " ms\n";
        stream << "GUI rendering: " << setprecision(4) << gui << " ms\n";
        stream << "Windows rendering: " << setprecision(4) << win << " ms\n";
        stream << "Swapping and events: " << setprecision(4) << buf << " ms\n";

        return stream.str();
    });
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(timingBox));

    // Log display
    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        auto logBox = make_shared<GuiTextBox>("Logs");
        logBox->setTextFunc([]()
        {
            int nbrLines = 10;
            // Convert the last lines of the text log
            vector<string> logs = Log::get().getLogs(Log::MESSAGE, Log::WARNING, Log::ERROR, Log::DEBUGGING);
            string text;
            int start = std::max(0, (int)logs.size() - nbrLines);
            for (int i = start; i < logs.size(); ++i)
            {
                if (i >= 0)
                    text += logs[i] + string("\n");
            }

            return text;
        });
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(logBox));
    }

    // Control
    auto controlView = make_shared<GuiControl>("Controls");
    controlView->setScene(_scene);
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(controlView));

    // Media
    auto mediaSelector = make_shared<GuiMedia>("Media");
    mediaSelector->setScene(_scene);
    _guiWidgets.push_back(mediaSelector);

    // GUI camera view
    auto globalView = make_shared<GuiGlobalView>("Cameras");
    globalView->setCamera(_guiCamera);
    globalView->setScene(_scene);
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(globalView));

    // Warp control
    auto warpControl = make_shared<GuiWarp>("Warp");
    warpControl->setScene(_scene);
    _guiWidgets.push_back(dynamic_pointer_cast<GuiWarp>(warpControl));

    // Performance graph
    if (Log::get().getVerbosity() == Log::DEBUGGING)
    {
        auto perfGraph = make_shared<GuiGraph>("Performance Graph");
        _guiWidgets.push_back(dynamic_pointer_cast<GuiWidget>(perfGraph));
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
    const float orthoProjection[4][4] = 
    {
        { 2.0f/width,	0.0f,			0.0f,		0.0f },
        { 0.0f,			2.0f/-height,	0.0f,		0.0f },
        { 0.0f,			0.0f,			-1.0f,		0.0f },
        { -1.0f,		1.0f,			0.0f,		1.0f },
    };

    glUseProgram(_imGuiShaderHandle);
    glUniform1i(_imGuiTextureLocation, 0);
    glUniformMatrix4fv(_imGuiProjMatrixLocation, 1, GL_FALSE, (float*)orthoProjection);
    glBindVertexArray(_imGuiVaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        //const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
        //glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        int needed_vtx_size = cmd_list->VtxBuffer.size() * sizeof(ImDrawVert);
        if (_imGuiVboMaxSize < needed_vtx_size)
        {
            _imGuiVboMaxSize = needed_vtx_size + 2000 * sizeof(ImDrawVert);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)_imGuiVboMaxSize, NULL, GL_STREAM_DRAW);
        }

        unsigned char* vtx_data = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, needed_vtx_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        if (!vtx_data)
            continue;
        memcpy(vtx_data, &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
        glUnmapBuffer(GL_ARRAY_BUFFER);

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
                glScissor((int)pcmd->ClipRect.x, (int)(height - pcmd->ClipRect.w),
                          (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
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
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

/*************/
void Gui::registerAttributes()
{
    _attribFunctions["size"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 2)
            return false;
        setOutputSize(args[0].asInt(), args[1].asInt());
        return true;
    });
}

} // end of namespace
