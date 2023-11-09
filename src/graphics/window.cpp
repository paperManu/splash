#include "./graphics/window.h"

#include <functional>

#include "GLFW/glfw3.h"

#include "./controller/controller_gui.h"
#include "./core/root_object.h"
#include "./core/scene.h"
#include "./graphics/api/window_gfx_impl.h"
#include "./graphics/camera.h"
#include "./graphics/geometry.h"
#include "./graphics/object.h"
#include "./graphics/renderer.h"
#include "./graphics/shader.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/warp.h"
#include "./image/image.h"
#include "./utils/log.h"
#include "./utils/scope_guard.h"
#include "./utils/timer.h"

using namespace std::placeholders;

namespace Splash
{

/*************/
std::mutex Window::_callbackMutex;
std::deque<std::pair<GLFWwindow*, std::vector<int>>> Window::_keys;
std::deque<std::pair<GLFWwindow*, unsigned int>> Window::_chars;
std::deque<std::pair<GLFWwindow*, std::vector<int>>> Window::_mouseBtn;
std::pair<GLFWwindow*, std::vector<double>> Window::_mousePos;
std::deque<std::pair<GLFWwindow*, std::vector<double>>> Window::_scroll;
std::vector<std::string> Window::_pathDropped;
std::atomic_bool Window::_quitFlag;

int Window::_swappableWindowsCount{0};

/*************/
Window::Window(RootObject* root)
    : GraphObject(root)
{
    _type = "window";
    _renderingPriority = Priority::WINDOW;
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    if (auto scene = dynamic_cast<Scene*>(root))
    {
        _gfxImpl = scene->getRenderer()->createWindowGfxImpl();
        _gfxImpl->init(scene);

        updateSwapInterval(scene->getSwapInterval());
    }

    _isInitialized = setProjectionSurface();
    if (!_isInitialized)
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while creating the Window" << Log::endl;
    else
        Log::get() << Log::MESSAGE << "Window::" << __FUNCTION__ << " - Window created successfully" << Log::endl;

    _viewProjectionMatrix = glm::ortho(-1.f, 1.f, -1.f, 1.f);

    setEventsCallbacks();
    showCursor(false);

    // Get the default window size and position
    glfwGetWindowPos(_gfxImpl->getGlfwWindow(), &_windowRect[0], &_windowRect[1]);
    glfwGetFramebufferSize(_gfxImpl->getGlfwWindow(), &_windowRect[2], &_windowRect[3]);
}

/*************/
Window::~Window()
{
    if (!_root)
        return;

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Window::~Window - Destructor" << Log::endl;
#endif
}

/*************/
int Window::getChar(GLFWwindow*& win, unsigned int& codepoint)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    if (_chars.size() == 0)
        return 0;

    win = _chars.front().first;
    codepoint = _chars.front().second;

    _chars.pop_front();

    return _chars.size() + 1;
}

/*************/
int Window::getKey(GLFWwindow*& win, int& key, int& action, int& mods)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    if (_keys.size() == 0)
        return 0;

    win = _keys.front().first;
    std::vector<int> key_pressed = _keys.front().second;

    key = key_pressed[0];
    action = key_pressed[2];
    mods = key_pressed[3];

    _keys.pop_front();

    return _keys.size() + 1;
}

/*************/
int Window::getMouseBtn(GLFWwindow*& win, int& btn, int& action, int& mods)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    if (_mouseBtn.size() == 0)
        return 0;

    win = _mouseBtn.front().first;
    std::vector<int> mouse = _mouseBtn.front().second;

    btn = mouse[0];
    action = mouse[1];
    mods = mouse[2];

    _mouseBtn.pop_front();

    return _mouseBtn.size() + 1;
}

/*************/
void Window::getMousePos(GLFWwindow*& win, int& xpos, int& ypos)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    if (_mousePos.second.size() != 2)
        return;

    win = _mousePos.first;
    xpos = (int)_mousePos.second[0];
    ypos = (int)_mousePos.second[1];
}

/*************/
int Window::getScroll(GLFWwindow*& win, double& xoffset, double& yoffset)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    if (_scroll.size() == 0)
        return 0;

    win = _scroll.front().first;
    xoffset = _scroll.front().second[0];
    yoffset = _scroll.front().second[1];

    _scroll.pop_front();

    return _scroll.size() + 1;
}

/*************/
std::vector<std::string> Window::getPathDropped()
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    auto paths = _pathDropped;
    _pathDropped.clear();
    return paths;
}

/*************/
bool Window::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Gui>(obj))
    {
        if (_gui)
            unlinkFrom(obj);

        _gui = std::dynamic_pointer_cast<Gui>(obj);

        if (const auto window = _gui->getOutputWindow(); window)
            window->unlinkFrom(obj);

        _gui->setOutputWindow(this);
        _guiTexture = _gui->getTexture();
        _screenGui->addTexture(_guiTexture);
        return true;
    }

    if (_guiOnly)
        return false;

    if (std::dynamic_pointer_cast<Texture>(obj))
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        setTexture(tex);
        return true;
    }
    else if (std::dynamic_pointer_cast<Image>(obj))
    {
        auto tex = std::dynamic_pointer_cast<Texture_Image>(_root->createObject("texture_image", getName() + "_" + obj->getName() + "_tex").lock());
        tex->setResizable(0);
        if (tex->linkTo(obj))
            return linkTo(tex);
        else
            return false;
    }
    else if (std::dynamic_pointer_cast<Camera>(obj))
    {
        auto scene = dynamic_cast<Scene*>(_root);
        if (!scene)
            return false;
        // Warps need to be duplicated in the master scene, to be available in the gui
        // So we create them asynchronously
        auto warpName = getName() + "_" + obj->getName() + "_warp";
        scene->sendMessageToWorld("sendAllScenes", {"addObject", "warp", warpName, _root->getName()});
        scene->sendMessageToWorld("sendAllScenes", {"link", obj->getName(), warpName});
        scene->sendMessageToWorld("sendAllScenes", {"link", warpName, _name});

        return true;
    }

    return false;
}

/*************/
void Window::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Texture>(obj))
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        unsetTexture(tex);
    }
    else if (std::dynamic_pointer_cast<Image>(obj))
    {
        // Look for the corresponding texture
        std::string texName = getName() + "_" + obj->getName() + "_tex";
        std::shared_ptr<Texture> tex = nullptr;
        for (auto& inTex : _inTextures)
        {
            if (inTex.expired())
                continue;
            auto lockedTex = inTex.lock();
            if (lockedTex->getName() == texName)
                tex = lockedTex;
        }
        if (tex != nullptr)
        {
            tex->unlinkFrom(obj);
            unlinkFrom(tex);
            _root->disposeObject(texName);
        }
    }
    else if (std::dynamic_pointer_cast<Camera>(obj))
    {
        auto warpName = getName() + "_" + obj->getName() + "_warp";

        if (auto warp = _root->getObject(warpName))
        {
            warp->unlinkFrom(obj);
            unlinkFrom(warp);
            _root->disposeObject(warpName);
        }
    }
    else if (std::dynamic_pointer_cast<Gui>(obj))
    {
        auto gui = std::dynamic_pointer_cast<Gui>(obj);
        if (gui->getTexture() == _guiTexture)
        {
            _screenGui->removeTexture(_guiTexture);
            _guiTexture = nullptr;
        }
    }
}

/*************/
void Window::updateSizeAndPos()
{
    int sizeAndPos[4];
    glfwGetWindowPos(_gfxImpl->getGlfwWindow(), &sizeAndPos[0], &sizeAndPos[1]);
    glfwGetFramebufferSize(_gfxImpl->getGlfwWindow(), &sizeAndPos[2], &sizeAndPos[3]);

    for (size_t i = 0; i < 4; ++i)
        if (sizeAndPos[i] != _windowRect[i])
            _resized = true;

    if (_resized)
        memcpy(_windowRect, sizeAndPos, 4 * sizeof(int));
}

/*************/
void Window::render()
{
#ifdef DEBUGGL
    _gfxImpl->setDebugData(reinterpret_cast<void*>(this));

    OnScopeExit
    {
        _gfxImpl->setDebugData(reinterpret_cast<void*>(_root));
    };
#endif

    // Update the window position and size
    updateSizeAndPos();

    // Update the FBO configuration if needed
    if (_resized)
    {
        _gfxImpl->setupFBOs(_scene, _windowRect);

        // It has to be created in the context where it is used, so its (re)creation will be triggered
        // in Window::swapBuffers if needed, depending on the following flag
        _renderTextureUpdated = true;
        _resized = false;
    }

    _gfxImpl->beginRender(_windowRect);

    // If we are in synchronization testing mode
    if (_swapSynchronizationTesting)
    {
        _gfxImpl->clearScreen(_swapSynchronizationColor, true);
    }
    // else, we draw the window normally
    else
    {
        _gfxImpl->clearScreen(glm::vec4(0.0, 0.0, 0.0, 1.0), false);

        auto layout = _layout;
        layout.push_front("_layout");
        _screen->activate();
        _screen->getShader()->setAttribute("uniform", layout);
        _screen->getShader()->setAttribute("uniform", {"_gamma", static_cast<float>(_srgb), _gammaCorrection});
        _screen->draw();
        _screen->deactivate();
    }

    if (_guiTexture != nullptr)
    {
        if (_guiOnly)
            _gui->setAttribute("fullscreen", {true});
        _screenGui->activate();
        _screenGui->draw();
        _screenGui->deactivate();
    }

    _gfxImpl->resetSyncFence();
    _swappableWindowsCount = 0; // Reset the window number

    // Resize the input textures accordingly to the window size.
    // This goes upstream to the cameras and gui
    // Textures are resized to the number of "frame" there are, according to the layout
    bool resize = true;
    for (uint32_t i = 0; i < _inTextures.size(); ++i)
    {
        int value = _layout[i].as<int>();
        for (uint32_t j = i + 1; j < _inTextures.size(); ++j)
            if (_layout[j].as<int>() != value)
                resize = false;
    }

    if (resize) // We don't do this if we are directly connected to a Texture (updated from an image)
    {
        for (auto& t : _inTextures)
        {
            if (t.expired())
                continue;
            t.lock()->setAttribute("size", {_windowRect[2], _windowRect[3]});
        }
    }
    if (_guiTexture != nullptr)
        _guiTexture->setAttribute("size", {_windowRect[2], _windowRect[3]});

    // Update the timestamp based on the input textures
    int64_t timestamp{0};
    for (auto& t : _inTextures)
    {
        auto tex = t.lock();
        if (!tex)
            continue;
        timestamp = std::max(timestamp, tex->getTimestamp());
    }
    _backBufferTimestamp = timestamp;

    _gfxImpl->endRender();

    return;
}

/*************/
void Window::swapBuffers()
{

    // Only one window will wait for vblank, the others draws directly into front buffer
    const auto windowIndex = _swappableWindowsCount++;

    _gfxImpl->swapBuffers(windowIndex, _srgb, _renderTextureUpdated, _windowRect);

    _frontBufferTimestamp = _backBufferTimestamp;
    _presentationDelay = Timer::getTime() - _frontBufferTimestamp;
}

/*************/
void Window::showCursor(bool visibility)
{
    const auto cursor = visibility ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN;
    glfwSetInputMode(_gfxImpl->getGlfwWindow(), GLFW_CURSOR, cursor);
}

/*************/
void Window::setTexture(const std::shared_ptr<Texture>& tex)
{
    auto textureIt = find_if(_inTextures.begin(), _inTextures.end(), [&](const std::weak_ptr<Texture>& t) {
        if (t.expired())
            return false;
        auto texture = t.lock();
        if (texture == tex)
            return true;
        return false;
    });

    if (textureIt != _inTextures.end())
        return;

    _inTextures.push_back(tex);
    _screen->addTexture(tex);
}

/*************/
void Window::unsetTexture(const std::shared_ptr<Texture>& tex)
{
    auto textureIt = find_if(_inTextures.begin(), _inTextures.end(), [&](const std::weak_ptr<Texture>& t) {
        if (t.expired())
            return false;
        auto texture = t.lock();
        if (texture == tex)
            return true;
        return false;
    });

    if (textureIt != _inTextures.end())
    {
        _inTextures.erase(textureIt);
        _screen->removeTexture(tex);
    }
}

/*************/
void Window::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    std::vector<int> keys{key, scancode, action, mods};
    _keys.push_back(std::pair<GLFWwindow*, std::vector<int>>(win, keys));
}

/*************/
void Window::charCallback(GLFWwindow* win, unsigned int codepoint)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    _chars.push_back(std::pair<GLFWwindow*, unsigned int>(win, codepoint));
}

/*************/
void Window::mouseBtnCallback(GLFWwindow* win, int button, int action, int mods)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    std::vector<int> btn{button, action, mods};
    _mouseBtn.push_back(std::pair<GLFWwindow*, std::vector<int>>(win, btn));
}

/*************/
void Window::mousePosCallback(GLFWwindow* win, double xpos, double ypos)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    std::vector<double> pos{xpos, ypos};
    _mousePos.first = win;
    _mousePos.second = std::move(pos);
}

/*************/
void Window::scrollCallback(GLFWwindow* win, double xoffset, double yoffset)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    std::vector<double> scroll{xoffset, yoffset};
    _scroll.push_back(std::pair<GLFWwindow*, std::vector<double>>(win, scroll));
}

/*************/
void Window::pathdropCallback(GLFWwindow* /*win*/, int count, const char** paths)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    for (int i = 0; i < count; ++i)
        _pathDropped.push_back(std::string(paths[i]));
}

/*************/
void Window::closeCallback(GLFWwindow* /*win*/)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    _quitFlag = true;
}

/*************/
void Window::setEventsCallbacks()
{
    glfwSetKeyCallback(_gfxImpl->getGlfwWindow(), Window::keyCallback);
    glfwSetCharCallback(_gfxImpl->getGlfwWindow(), Window::charCallback);
    glfwSetMouseButtonCallback(_gfxImpl->getGlfwWindow(), Window::mouseBtnCallback);
    glfwSetCursorPosCallback(_gfxImpl->getGlfwWindow(), Window::mousePosCallback);
    glfwSetScrollCallback(_gfxImpl->getGlfwWindow(), Window::scrollCallback);
    glfwSetDropCallback(_gfxImpl->getGlfwWindow(), Window::pathdropCallback);
    glfwSetWindowCloseCallback(_gfxImpl->getGlfwWindow(), Window::closeCallback);
}

/*************/
bool Window::setProjectionSurface()
{
    _gfxImpl->setAsCurrentContext();
    glfwShowWindow(_gfxImpl->getGlfwWindow());
    glfwSwapInterval(_swapInterval);

// Setup the projection surface
#ifdef DEBUG
    glGetError();
#endif

    _screen = std::make_shared<Object>(_root);
    _screen->setAttribute("fill", {"window"});
    auto virtualScreen = _renderer->createGeometry(_root);
    _screen->addGeometry(virtualScreen);

    _screenGui = std::make_shared<Object>(_root);
    _screenGui->setAttribute("fill", {"window"});
    virtualScreen = _renderer->createGeometry(_root);
    _screenGui->addGeometry(virtualScreen);

#ifdef DEBUG
    GLenum error = glGetError();
    if (error)
        Log::get() << Log::WARNING << __FUNCTION__ << " - Error while creating the projection surface: " << error << Log::endl;
#endif

    _gfxImpl->releaseContext();

#ifdef DEBUG
    return error == 0 ? true : false;
#else
    return true;
#endif
}

/*************/
void Window::setWindowDecoration(bool hasDecoration)
{
    if (glfwGetCurrentContext() == nullptr)
        return;

    glfwWindowHint(GLFW_VISIBLE, true);
    glfwWindowHint(GLFW_RESIZABLE, hasDecoration);
    glfwWindowHint(GLFW_DECORATED, hasDecoration);
    GLFWwindow* window;
    window = glfwCreateWindow(_windowRect[2], _windowRect[3], ("Splash::" + _name).c_str(), 0, _gfxImpl->getMainWindow());

    // Reset hints to default ones
    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_DECORATED, true);

    if (!window)
    {
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Unable to update window " << _name << Log::endl;
        return;
    }

    _gfxImpl->updateGlfwWindow(window);
    updateSwapInterval(_swapInterval);
    _resized = true;

    setEventsCallbacks();
    showCursor(false);

    return;
}

/*************/
void Window::updateSwapInterval(int swapInterval)
{
    if (!_gfxImpl->windowExists())
        return;

    _gfxImpl->setAsCurrentContext();

    _swapInterval = std::max<int>(-1, swapInterval);
    glfwSwapInterval(_swapInterval);

    _gfxImpl->releaseContext();
}

/*************/
void Window::updateWindowShape()
{
    if (!_gfxImpl || !_gfxImpl->windowExists())
        return;

    bool wasGlfwContextEnabled = true;
    if (!_gfxImpl->isCurrentContext())
    {
        wasGlfwContextEnabled = false;
        _gfxImpl->setAsCurrentContext();
    }

    glfwSetWindowPos(_gfxImpl->getGlfwWindow(), _windowRect[0], _windowRect[1]);
    glfwSetWindowSize(_gfxImpl->getGlfwWindow(), _windowRect[2], _windowRect[3]);

    if (!wasGlfwContextEnabled)
        _gfxImpl->releaseContext();
}

/*************/
void Window::registerAttributes()
{
    GraphObject::registerAttributes();

    addAttribute(
        "decorated",
        [&](const Values& args) {
            _withDecoration = args[0].as<bool>();
            setWindowDecoration(_withDecoration);
            updateWindowShape();
            return true;
        },
        [&]() -> Values { return {_withDecoration}; },
        {'b'});
    setAttributeDescription("decorated", "If set to 0, the window is drawn without decoration");

    addAttribute(
        "guiOnly",
        [&](const Values& args) {
            _guiOnly = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_guiOnly}; },
        {'b'});
    setAttributeDescription("guiOnly", "If true, only the GUI will be able to link to this window. Does not affect pre-existing links.");

    addAttribute(
        "srgb",
        [&](const Values& args) {
            _srgb = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_srgb}; },
        {'b'});
    setAttributeDescription("srgb", "If true, the window is drawn in the sRGB color space");

    addAttribute(
        "gamma",
        [&](const Values& args) {
            _gammaCorrection = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_gammaCorrection}; },
        {'r'});
    setAttributeDescription("gamma", "Set the gamma correction for this window");

    // Attribute to configure the placement of the various texture input
    addAttribute(
        "layout",
        [&](const Values& args) {
            _layout.clear();
            for (auto& arg : args)
                _layout.push_back(arg.as<int>());

            for (int i = _layout.size(); i < 4; ++i)
                _layout.push_back(0);

            if (_layout.size() > 4)
                _layout.resize(4);

            return true;
        },
        [&]() { return _layout; },
        {'i'});
    setAttributeDescription("layout", "Set the placement of the various input textures");

    addAttribute(
        "position",
        [&](const Values& args) {
            _windowRect[0] = args[0].as<int>();
            _windowRect[1] = args[1].as<int>();
            updateWindowShape();
            return true;
        },
        [&]() -> Values {
            return {_windowRect[0], _windowRect[1]};
        },
        {'i', 'i'});
    setAttributeDescription("position", "Set the window position");

    addAttribute("showCursor",
        [&](const Values& args) {
            showCursor(args[0].as<bool>());
            return true;
        },
        {'b'});

    addAttribute(
        "size",
        [&](const Values& args) {
            _windowRect[2] = args[0].as<int>();
            _windowRect[3] = args[1].as<int>();
            _resized = true;
            updateWindowShape();
            return true;
        },
        [&]() -> Values {
            return {_windowRect[2], _windowRect[3]};
        },
        {'i', 'i'});
    setAttributeDescription("size", "Set the window dimensions");

    addAttribute("swapInterval",
        [&](const Values& args) {
            updateSwapInterval(args[0].as<int>());
            return true;
        },
        {'i'});
    setAttributeDescription("swapInterval", "Set the window swap interval");

    addAttribute("swapTest",
        [&](const Values& args) {
            _swapSynchronizationTesting = args[0].as<bool>();
            return true;
        },
        {'b'});
    setAttributeDescription("swapTest", "Activate video swap test if true");

    addAttribute("swapTestColor",
        [&](const Values& args) {
            _swapSynchronizationColor = glm::vec4(args[0].as<float>(), args[1].as<float>(), args[2].as<float>(), args[3].as<float>());
            return true;
        },
        {'r', 'r', 'r', 'r'});
    setAttributeDescription("swapTestColor", "Set the swap test color");

    addAttribute("textureList", [&]() -> Values {
        Values textureList;
        for (const auto& layout_index : _layout)
        {
            auto index = layout_index.as<size_t>();
            if (index >= _inTextures.size())
                continue;

            auto texture = _inTextures[index].lock();
            if (!texture)
            {
                textureList.push_back("");
                continue;
            }

            textureList.push_back(texture->getName());
        }
        return textureList;
    });
    setAttributeDescription("textureList", "Get the list of the textures linked to the window");

    addAttribute("presentationDelay", [&]() -> Values { return {_presentationDelay}; });
    setAttributeDescription("presentationDelay", "Delay between the update of an image and its display");
}

} // namespace Splash
