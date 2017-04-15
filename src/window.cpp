#include "./window.h"

#include "./camera.h"
#include "./controller_gui.h"
#include "./geometry.h"
#include "./image.h"
#include "./log.h"
#include "./object.h"
#include "./scene.h"
#include "./shader.h"
#include "./texture.h"
#include "./texture_image.h"
#include "./timer.h"
#include "./warp.h"

#include <functional>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace std::placeholders;

namespace Splash
{

/*************/
mutex Window::_callbackMutex;
deque<pair<GLFWwindow*, vector<int>>> Window::_keys;
deque<pair<GLFWwindow*, unsigned int>> Window::_chars;
deque<pair<GLFWwindow*, vector<int>>> Window::_mouseBtn;
pair<GLFWwindow*, vector<double>> Window::_mousePos;
deque<pair<GLFWwindow*, vector<double>>> Window::_scroll;
vector<string> Window::_pathDropped;
atomic_bool Window::_quitFlag;

atomic_int Window::_swappableWindowsCount{0};

/*************/
Window::Window(RootObject* root)
    : BaseObject(root)
{
    _type = "window";
    _renderingPriority = Priority::WINDOW;
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    if (auto scene = dynamic_cast<Scene*>(root))
    {
        auto w = scene->getNewSharedWindow();
        if (w.get() == nullptr)
            return;
        _window = w;
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
    glfwGetWindowPos(_window->get(), &_windowRect[0], &_windowRect[1]);
    glfwGetFramebufferSize(_window->get(), &_windowRect[2], &_windowRect[3]);

    // Create the render FBO
    glGetError();
    glGenFramebuffers(1, &_renderFbo);
    setupRenderFBO();

    glBindFramebuffer(GL_FRAMEBUFFER, _renderFbo);
    GLenum _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << _status << Log::endl;
    else
        Log::get() << Log::MESSAGE << "Window::" << __FUNCTION__ << " - Render framebuffer object successfully initialized" << Log::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // And the read framebuffer
    setupReadFBO();
}

/*************/
Window::~Window()
{
    if (!_root)
        return;

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Window::~Window - Destructor" << Log::endl;
#endif

    glDeleteFramebuffers(1, &_renderFbo);
    glDeleteFramebuffers(1, &_readFbo);
}

/*************/
int Window::getChars(GLFWwindow*& win, unsigned int& codepoint)
{
    lock_guard<mutex> lock(_callbackMutex);
    if (_chars.size() == 0)
        return 0;

    win = _chars.front().first;
    codepoint = _chars.front().second;

    _chars.pop_front();

    return _chars.size() + 1;
}

/*************/
int Window::getKeys(GLFWwindow*& win, int& key, int& action, int& mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    if (_keys.size() == 0)
        return 0;

    win = _keys.front().first;
    vector<int> keys = _keys.front().second;

    key = keys[0];
    action = keys[2];
    mods = keys[3];

    _keys.pop_front();

    return _keys.size() + 1;
}

/*************/
int Window::getMouseBtn(GLFWwindow*& win, int& btn, int& action, int& mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    if (_mouseBtn.size() == 0)
        return 0;

    win = _mouseBtn.front().first;
    vector<int> mouse = _mouseBtn.front().second;

    btn = mouse[0];
    action = mouse[1];
    mods = mouse[2];

    _mouseBtn.pop_front();

    return _mouseBtn.size() + 1;
}

/*************/
void Window::getMousePos(GLFWwindow*& win, int& xpos, int& ypos)
{
    lock_guard<mutex> lock(_callbackMutex);
    if (_mousePos.second.size() != 2)
        return;

    win = _mousePos.first;
    xpos = (int)_mousePos.second[0];
    ypos = (int)_mousePos.second[1];
}

/*************/
int Window::getScroll(GLFWwindow*& win, double& xoffset, double& yoffset)
{
    lock_guard<mutex> lock(_callbackMutex);
    if (_scroll.size() == 0)
        return 0;

    win = _scroll.front().first;
    xoffset = _scroll.front().second[0];
    yoffset = _scroll.front().second[1];

    _scroll.pop_front();

    return _scroll.size() + 1;
}

/*************/
vector<string> Window::getPathDropped()
{
    lock_guard<mutex> lock(_callbackMutex);
    auto paths = _pathDropped;
    _pathDropped.clear();
    return paths;
}

/*************/
bool Window::linkTo(const shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    if (!BaseObject::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        setTexture(tex);
        return true;
    }
    else if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        auto tex = dynamic_pointer_cast<Texture_Image>(_root->createObject("texture_image", getName() + "_" + obj->getName() + "_tex"));
        tex->setResizable(0);
        if (tex->linkTo(obj))
            return linkTo(tex);
        else
            return false;
    }
    else if (dynamic_pointer_cast<Camera>(obj).get() != nullptr)
    {
        auto scene = dynamic_cast<Scene*>(_root);
        // Warps need to be duplicated in the master scene, to be available in the gui
        if (scene && !scene->isMaster())
        {
            auto warpName = getName() + "_" + obj->getName() + "_warp";
            scene->sendMessageToWorld("sendToMasterScene", {"addGhost", "warp", warpName});
            scene->sendMessageToWorld("sendToMasterScene", {"linkGhost", obj->getName(), warpName});
            scene->sendMessageToWorld("sendToMasterScene", {"linkGhost", warpName, _name});
        }

        auto warp = dynamic_pointer_cast<Warp>(_root->createObject("warp", getName() + "_" + obj->getName() + "_warp"));
        if (warp->linkTo(obj))
            return linkTo(warp);

        return false;
    }
    else if (dynamic_pointer_cast<Gui>(obj).get() != nullptr)
    {
        if (_guiTexture != nullptr)
            _screenGui->removeTexture(_guiTexture);
        auto gui = dynamic_pointer_cast<Gui>(obj);
        _guiTexture = gui->getTexture();
        _screenGui->addTexture(_guiTexture);
        return true;
    }

    return false;
}

/*************/
void Window::unlinkFrom(const shared_ptr<BaseObject>& obj)
{
    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        unsetTexture(tex);
    }
    else if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        // Look for the corresponding texture
        string texName = getName() + "_" + obj->getName() + "_tex";
        shared_ptr<Texture> tex = nullptr;
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
            unsetTexture(tex);
            tex.reset();
            _root->disposeObject(texName);
        }
    }
    else if (dynamic_pointer_cast<Camera>(obj).get() != nullptr)
    {
        auto warpName = getName() + "_" + obj->getName() + "_warp";

        if (auto warp = _root->getObject(warpName))
        {
            warp->unlinkFrom(obj);
            unlinkFrom(warp);
        }

        _root->disposeObject(warpName);

        auto cam = dynamic_pointer_cast<Camera>(obj);
        for (auto& tex : cam->getTextures())
            unsetTexture(tex);
    }
    else if (dynamic_pointer_cast<Gui>(obj).get() != nullptr)
    {
        auto gui = dynamic_pointer_cast<Gui>(obj);
        if (gui->getTexture() == _guiTexture)
        {
            _screenGui->removeTexture(_guiTexture);
            _guiTexture = nullptr;
        }
    }

    return BaseObject::unlinkFrom(obj);
}

/*************/
void Window::render()
{
    // Update the FBO configuration if needed
    setupRenderFBO();

    int w, h;
    glfwGetFramebufferSize(_window->get(), &w, &h);
    glViewport(0, 0, w, h);

#ifdef DEBUG
    glGetError();
#endif

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);
    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, fboBuffers);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (_srgb)
        glEnable(GL_FRAMEBUFFER_SRGB);

    // If we are in synchronization testing mode
    if (_swapSynchronizationTesting)
    {
        glClearColor(_swapSynchronizationColor[0], _swapSynchronizationColor[1], _swapSynchronizationColor[2], _swapSynchronizationColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    // else, we draw the window normally
    else
    {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        auto layout = _layout;
        layout.push_front("layout");
        _screen->getShader()->setAttribute("uniform", layout);
        _screen->getShader()->setAttribute("uniform", {"_gamma", (float)_srgb, _gammaCorrection});
        _screen->activate();
        _screen->draw();
        _screen->deactivate();
    }

    if (_guiTexture != nullptr)
    {
        _screenGui->activate();
        _screenGui->draw();
        _screenGui->deactivate();
    }

    glDeleteSync(_renderFence);
    _renderFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    _swappableWindowsCount = 0; // Reset the window number

    // Resize the input textures accordingly to the window size.
    // This goes upstream to the cameras and gui
    // Textures are resized to the number of "frame" there are, according to the layout
    bool resize = true;
    for (int i = 0; i < _inTextures.size(); ++i)
    {
        int value = _layout[i].as<int>();
        for (int j = i + 1; j < _inTextures.size(); ++j)
            if (_layout[j].as<int>() != value)
                resize = false;
    }
    if (resize) // We don't do this if we are directly connected to a Texture (updated from an image)
    {
        for (auto& t : _inTextures)
        {
            if (t.expired())
                continue;
            t.lock()->setAttribute("size", {w, h});
        }
    }
    if (_guiTexture != nullptr)
        _guiTexture->setAttribute("size", {w, h});

#ifdef DEBUG
    GLenum error = glGetError();
    if (error)
        Log::get() << Log::WARNING << _type << "::" << __FUNCTION__ << " - Error while rendering the window: " << error << Log::endl;
#endif

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    return;
}

/*************/
void Window::setupRenderFBO()
{
    glfwGetFramebufferSize(_window->get(), &_windowRect[2], &_windowRect[3]);

    glBindFramebuffer(GL_FRAMEBUFFER, _renderFbo);

    if (!_depthTexture)
    {
        _depthTexture = make_shared<Texture_Image>(_root, 512, 512, "D", nullptr);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    }
    else
    {
        _depthTexture->setResizable(1);
        _depthTexture->setAttribute("size", {_windowRect[2], _windowRect[3]});
        _depthTexture->setResizable(0);
    }

    if (!_colorTexture)
    {
        _colorTexture = make_shared<Texture_Image>(_root);
        _colorTexture->setAttribute("filtering", {0});
        _colorTexture->reset(_windowRect[2], _windowRect[3], "sRGBA", nullptr);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);
    }
    else
    {
        _colorTexture->setResizable(1);
        _colorTexture->setAttribute("size", {_windowRect[2], _windowRect[3]});
        _colorTexture->setResizable(0);
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*************/
void Window::setupReadFBO()
{
    _window->setAsCurrentContext();

    if (_readFbo != 0)
        glDeleteFramebuffers(1, &_readFbo);

    glGenFramebuffers(1, &_readFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, _readFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);
    GLenum _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while initializing read framebuffer object: " << _status << Log::endl;
    else
        Log::get() << Log::MESSAGE << "Window::" << __FUNCTION__ << " - Read framebuffer object successfully initialized" << Log::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    _window->releaseContext();
}

/*************/
void Window::swapBuffers()
{
    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;

    glFlush();
    glWaitSync(_renderFence, 0, GL_TIMEOUT_IGNORED);

    // Only one window will wait for vblank, the others draws directly into front buffer
    auto windowIndex = _swappableWindowsCount.fetch_add(1);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _readFbo);

// If swap interval is null (meaning no vsync), draw directly to the front buffer in any case
#if HAVE_OSX
    glDrawBuffer(GL_BACK);
#else
    auto drawBuffer = GL_BACK;
    if (!Scene::getHasNVSwapGroup() && windowIndex != 0)
        drawBuffer = GL_FRONT;
    glDrawBuffer(drawBuffer);
#endif

    glBlitFramebuffer(0, 0, _windowRect[2], _windowRect[3], 0, 0, _windowRect[2], _windowRect[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

#if HAVE_OSX
    glfwSwapBuffers(_window->get());
#else
    if (Scene::getHasNVSwapGroup())
        glfwSwapBuffers(_window->get());
    else if (windowIndex == 0)
        glfwSwapBuffers(_window->get());
#endif

    _window->releaseContext();
}

/*************/
void Window::showCursor(bool visibility)
{
    if (visibility)
        glfwSetInputMode(_window->get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(_window->get(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

/*************/
bool Window::switchFullscreen(int screenId)
{
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (screenId >= count)
        return false;

    if (_window.get() == nullptr)
        return false;

    if (screenId == _screenId)
        return true;
    _screenId = screenId;

    glfwWindowHint(GLFW_VISIBLE, true);
    GLFWwindow* window;
    if (_screenId != -1 && glfwGetWindowMonitor(_window->get()) == NULL)
    {
        const GLFWvidmode* vidmode = glfwGetVideoMode(monitors[_screenId]);
        glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);

        window = glfwCreateWindow(vidmode->width, vidmode->height, ("Splash::" + _name).c_str(), monitors[_screenId], _window->getMainWindow());
    }
    else
    {
        int width, height;
        glfwGetWindowSize(_window->get(), &width, &height);
        window = glfwCreateWindow(width, height, ("Splash::" + _name).c_str(), 0, _window->getMainWindow());
    }

    if (!window)
    {
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Unable to create new fullscreen shared window" << Log::endl;
        return false;
    }

    _window = move(make_shared<GlWindow>(window, _window->getMainWindow()));
    updateSwapInterval();
    setupReadFBO();

    setEventsCallbacks();
    showCursor(false);

    return true;
}

/*************/
void Window::setTexture(const shared_ptr<Texture>& tex)
{
    auto textureIt = find_if(_inTextures.begin(), _inTextures.end(), [&](const weak_ptr<Texture>& t) {
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
void Window::unsetTexture(const shared_ptr<Texture>& tex)
{
    auto textureIt = find_if(_inTextures.begin(), _inTextures.end(), [&](const weak_ptr<Texture>& t) {
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
    lock_guard<mutex> lock(_callbackMutex);
    vector<int> keys{key, scancode, action, mods};
    _keys.push_back(pair<GLFWwindow*, vector<int>>(win, keys));
}

/*************/
void Window::charCallback(GLFWwindow* win, unsigned int codepoint)
{
    lock_guard<mutex> lock(_callbackMutex);
    _chars.push_back(pair<GLFWwindow*, unsigned int>(win, codepoint));
}

/*************/
void Window::mouseBtnCallback(GLFWwindow* win, int button, int action, int mods)
{
    lock_guard<mutex> lock(_callbackMutex);
    vector<int> btn{button, action, mods};
    _mouseBtn.push_back(pair<GLFWwindow*, vector<int>>(win, btn));
}

/*************/
void Window::mousePosCallback(GLFWwindow* win, double xpos, double ypos)
{
    lock_guard<mutex> lock(_callbackMutex);
    vector<double> pos{xpos, ypos};
    _mousePos.first = win;
    _mousePos.second = move(pos);
}

/*************/
void Window::scrollCallback(GLFWwindow* win, double xoffset, double yoffset)
{
    lock_guard<mutex> lock(_callbackMutex);
    vector<double> scroll{xoffset, yoffset};
    _scroll.push_back(pair<GLFWwindow*, vector<double>>(win, scroll));
}

/*************/
void Window::pathdropCallback(GLFWwindow* win, int count, const char** paths)
{
    lock_guard<mutex> lock(_callbackMutex);
    for (int i = 0; i < count; ++i)
        _pathDropped.push_back(string(paths[i]));
}

/*************/
void Window::closeCallback(GLFWwindow* win)
{
    lock_guard<mutex> lock(_callbackMutex);
    _quitFlag = true;
}

/*************/
void Window::setEventsCallbacks()
{
    glfwSetKeyCallback(_window->get(), Window::keyCallback);
    glfwSetCharCallback(_window->get(), Window::charCallback);
    glfwSetMouseButtonCallback(_window->get(), Window::mouseBtnCallback);
    glfwSetCursorPosCallback(_window->get(), Window::mousePosCallback);
    glfwSetScrollCallback(_window->get(), Window::scrollCallback);
    glfwSetDropCallback(_window->get(), Window::pathdropCallback);
    glfwSetWindowCloseCallback(_window->get(), Window::closeCallback);
}

/*************/
bool Window::setProjectionSurface()
{
    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;
    ;
    glfwShowWindow(_window->get());
    glfwSwapInterval(_swapInterval);

// Setup the projection surface
#ifdef DEBUG
    glGetError();
#endif

    _screen = make_shared<Object>(_root);
    _screen->setAttribute("fill", {"window"});
    auto virtualScreen = make_shared<Geometry>(_root);
    _screen->addGeometry(virtualScreen);

    _screenGui = make_shared<Object>(_root);
    _screenGui->setAttribute("fill", {"window"});
    virtualScreen = make_shared<Geometry>(_root);
    _screenGui->addGeometry(virtualScreen);

#ifdef DEBUG
    GLenum error = glGetError();
    if (error)
        Log::get() << Log::WARNING << __FUNCTION__ << " - Error while creating the projection surface: " << error << Log::endl;
#endif

    _window->releaseContext();

#ifdef DEBUG
    return error == 0 ? true : false;
#else
    return true;
#endif
}

/*************/
void Window::setWindowDecoration(bool hasDecoration)
{
    if (_screenId != -1)
        return;

    glfwWindowHint(GLFW_VISIBLE, true);
    glfwWindowHint(GLFW_RESIZABLE, hasDecoration);
    glfwWindowHint(GLFW_DECORATED, hasDecoration);
    GLFWwindow* window;
    window = glfwCreateWindow(_windowRect[2], _windowRect[3], ("Splash::" + _name).c_str(), 0, _window->getMainWindow());

    // Reset hints to default ones
    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_DECORATED, true);

    if (!window)
    {
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Unable to update window " << _name << Log::endl;
        return;
    }

    _window = move(make_shared<GlWindow>(window, _window->getMainWindow()));
    updateSwapInterval();
    setupRenderFBO();
    setupReadFBO();

    setEventsCallbacks();
    showCursor(false);

    return;
}

/*************/
void Window::updateSwapInterval()
{
    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;

    glfwSwapInterval(_swapInterval);

    _window->releaseContext();
}

/*************/
void Window::updateWindowShape()
{
    if (_screenId == -1)
    {
        glfwSetWindowPos(_window->get(), _windowRect[0], _windowRect[1]);
        glfwSetWindowSize(_window->get(), _windowRect[2], _windowRect[3]);
    }
}

/*************/
void Window::registerAttributes()
{
    BaseObject::registerAttributes();

    addAttribute("fullscreen",
        [&](const Values& args) {
            switchFullscreen(args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_screenId}; },
        {'n'});
    setAttributeDescription("fullscreen", "Set the window as fullscreen given the screen index");

    addAttribute("decorated",
        [&](const Values& args) {
            _withDecoration = args[0].as<int>() == 0 ? false : true;
            setWindowDecoration(_withDecoration);
            updateWindowShape();
            return true;
        },
        [&]() -> Values { return {static_cast<int>(_withDecoration)}; },
        {'n'});
    setAttributeDescription("decorated", "If set to 0, the window is drawn without decoration");

    addAttribute("srgb",
        [&](const Values& args) {
            if (args[0].as<int>() != 0)
                _srgb = true;
            else
                _srgb = false;
            return true;
        },
        [&]() -> Values { return {_srgb}; },
        {'n'});
    setAttributeDescription("srgb", "If set to 1, the window is drawn in the sRGB color space");

    addAttribute("gamma",
        [&](const Values& args) {
            _gammaCorrection = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_gammaCorrection}; },
        {'n'});
    setAttributeDescription("gamma", "Set the gamma correction for this window");

    // Attribute to configure the placement of the various texture input
    addAttribute("layout",
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
        {'n'});
    setAttributeDescription("layout", "Set the placement of the various input textures");

    addAttribute("position",
        [&](const Values& args) {
            _windowRect[0] = args[0].as<int>();
            _windowRect[1] = args[1].as<int>();
            updateWindowShape();
            return true;
        },
        [&]() -> Values {
            return {_windowRect[0], _windowRect[1]};
        },
        {'n', 'n'});
    setAttributeDescription("position", "Set the window position");

    addAttribute("showCursor",
        [&](const Values& args) {
            showCursor(args[0].as<int>());
            return true;
        },
        {'n'});

    addAttribute("size",
        [&](const Values& args) {
            _windowRect[2] = args[0].as<int>();
            _windowRect[3] = args[1].as<int>();
            updateWindowShape();
            return true;
        },
        [&]() -> Values {
            return {_windowRect[2], _windowRect[3]};
        },
        {'n', 'n'});
    setAttributeDescription("size", "Set the window dimensions");

    addAttribute("swapInterval",
        [&](const Values& args) {
            _swapInterval = max(-1, args[0].as<int>());
            updateSwapInterval();
            return true;
        },
        {'n'});
    setAttributeDescription("swapInterval", "Set the window swap interval");

    addAttribute("swapTest",
        [&](const Values& args) {
            _swapSynchronizationTesting = args[0].as<int>();
            return true;
        },
        {'n'});
    setAttributeDescription("swapTest", "Activate video swap test if set to 1");

    addAttribute("swapTestColor",
        [&](const Values& args) {
            _swapSynchronizationColor = glm::vec4(args[0].as<float>(), args[1].as<float>(), args[2].as<float>(), args[3].as<float>());
            return true;
        },
        {'n', 'n', 'n', 'n'});
    setAttributeDescription("swapTestColor", "Set the swap test color");
}

} // end of namespace
