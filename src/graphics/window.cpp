#include "./graphics/window.h"

#include "./controller/controller_gui.h"
#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/geometry.h"
#include "./graphics/object.h"
#include "./graphics/shader.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./graphics/warp.h"
#include "./image/image.h"
#include "./utils/log.h"
#include "./utils/timer.h"

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
    glfwGetWindowPos(_window->get(), &_windowRect[0], &_windowRect[1]);
    glfwGetFramebufferSize(_window->get(), &_windowRect[2], &_windowRect[3]);
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
        if (!scene)
            return false;
        // Warps need to be duplicated in the master scene, to be available in the gui
        // So we create them asynchronously
        auto warpName = getName() + "_" + obj->getName() + "_warp";
        scene->sendMessageToWorld("sendAllScenes", {"add", "warp", warpName, _root->getName()});
        scene->sendMessageToWorld("sendAllScenes", {"link", obj->getName(), warpName});
        scene->sendMessageToWorld("sendAllScenes", {"link", warpName, _name});

        return true;
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
        unsetTexture(cam->getTexture());
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
    // Get the current window size
    int w, h;
    glfwGetFramebufferSize(_window->get(), &w, &h);
    if (w != _windowRect[2] || h != _windowRect[3])
    {
        _resized = true;
        _windowRect[2] = w;
        _windowRect[3] = h;
    }

    // Update the FBO configuration if needed
    if (_resized)
    {
        setupFBOs();
        _resized = false;
    }

    glfwGetFramebufferSize(_window->get(), &w, &h);
    glViewport(0, 0, w, h);

#ifdef DEBUG
    glGetError();
#endif

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);
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
        glClearColor(0.0, 0.0, 0.0, 1.0);
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
    _swappableWindowsCount.store(0, std::memory_order_acq_rel); // Reset the window number

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
    glDisable(GL_FRAMEBUFFER_SRGB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    return;
}

/*************/
void Window::setupFBOs()
{
    // Render FBO
    if (glIsFramebuffer(_renderFbo) == GL_FALSE)
        glCreateFramebuffers(1, &_renderFbo);

    glNamedFramebufferTexture(_renderFbo, GL_DEPTH_ATTACHMENT, 0, 0);
    _depthTexture = make_shared<Texture_Image>(_root, _windowRect[2], _windowRect[3], "D", nullptr);
    glNamedFramebufferTexture(_renderFbo, GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);

    glNamedFramebufferTexture(_renderFbo, GL_COLOR_ATTACHMENT0, 0, 0);
    _colorTexture = make_shared<Texture_Image>(_root);
    _colorTexture->setAttribute("filtering", {0});
    _colorTexture->reset(_windowRect[2], _windowRect[3], "sRGBA", nullptr);
    glNamedFramebufferTexture(_renderFbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);

    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glNamedFramebufferDrawBuffers(_renderFbo, 1, fboBuffers);

    GLenum _status = glCheckNamedFramebufferStatus(_renderFbo, GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << _status << Log::endl;
    else
        Log::get() << Log::DEBUGGING << "Window::" << __FUNCTION__ << " - Render framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Read FBO
    if (_readFbo != 0)
        glDeleteFramebuffers(1, &_readFbo);

    glCreateFramebuffers(1, &_readFbo);

    glNamedFramebufferTexture(_readFbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
    _status = glCheckNamedFramebufferStatus(_readFbo, GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while initializing read framebuffer object: " << _status << Log::endl;
    else
        Log::get() << Log::DEBUGGING << "Window::" << __FUNCTION__ << " - Read framebuffer object successfully initialized" << Log::endl;
}

/*************/
void Window::swapBuffers()
{
    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;

    glWaitSync(_renderFence, 0, GL_TIMEOUT_IGNORED);

    // Only one window will wait for vblank, the others draws directly into front buffer
    auto windowIndex = _swappableWindowsCount.fetch_add(1, std::memory_order_acq_rel);

// If swap interval is null (meaning no vsync), draw directly to the front buffer in any case
#if not HAVE_OSX
    bool drawToFront = false;
    if (!Scene::getHasNVSwapGroup() && windowIndex != 0)
    {
        drawToFront = true;
        glDrawBuffer(GL_FRONT);
    }
#endif

    glBlitNamedFramebuffer(_readFbo, 0, 0, 0, _windowRect[2], _windowRect[3], 0, 0, _windowRect[2], _windowRect[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if HAVE_OSX
    glfwSwapBuffers(_window->get());
#else
    if (Scene::getHasNVSwapGroup())
        glfwSwapBuffers(_window->get());
    else if (windowIndex == 0)
        glfwSwapBuffers(_window->get());
#endif

    if (drawToFront)
        glDrawBuffer(GL_BACK);

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
void Window::pathdropCallback(GLFWwindow* /*win*/, int count, const char** paths)
{
    lock_guard<mutex> lock(_callbackMutex);
    for (int i = 0; i < count; ++i)
        _pathDropped.push_back(string(paths[i]));
}

/*************/
void Window::closeCallback(GLFWwindow* /*win*/)
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
    updateSwapInterval(_swapInterval);
    _resized = true;

    setEventsCallbacks();
    showCursor(false);

    return;
}

/*************/
void Window::updateSwapInterval(int swapInterval)
{
    if (!_window->setAsCurrentContext())
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;

    _swapInterval = max<int>(-1, swapInterval);
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
            _resized = true;
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
            updateSwapInterval(args[0].as<int>());
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
