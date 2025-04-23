#include "./graphics/rendering_context.h"

#include <algorithm>
#include <iostream>
#include <stdlib.h>

#include <GLFW/glfw3.h>

#include "./graphics/window.h"

#define DEFAULT_WINDOW_WIDTH 1024
#define DEFAULT_WINDOW_HEIGHT 768

namespace Splash
{

bool RenderingContext::_glfwInitialized = false;
bool RenderingContext::_glfwPlatformWayland = false;
std::vector<GLFWmonitor*> RenderingContext::_monitors;

/*************/
RenderingContext::RenderingContext(std::string_view name, const gfx::PlatformVersion& platformVersion)
{
    if (!_glfwInitialized)
    {
        glfwSetErrorCallback(glfwErrorCallback);

        // By default, GLFW will prefer using X11 as Wayland support is not complete
        // Using Wayland can be forced with the SPLASH_USE_WAYLAND env var
        if (getenv(Constants::SPLASH_USE_WAYLAND))
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

        if (!glfwInit())
        {
            Log::get() << Log::ERROR << "RenderingContext::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
            return;
        }

        glfwSetMonitorCallback(glfwMonitorCallback);
        _glfwInitialized = true;
        _glfwPlatformWayland = (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND);
    }

    _monitors = getMonitorList();
    _name = std::string(name);

    if (platformVersion.name == "OpenGL")
    {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
    }
    else if (platformVersion.name == "OpenGL ES")
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    }

#ifdef DEBUGGL
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, static_cast<int>(platformVersion.major));
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, static_cast<int>(platformVersion.minor));
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    _window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, name.data(), nullptr, nullptr);
}

/*************/
RenderingContext::RenderingContext(std::string_view name, RenderingContext* context)
{
    assert(context != nullptr);
    _name = std::string(name);
    _window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, name.data(), nullptr, context->_window);
    _mainContext = context;
}

/*************/
RenderingContext::~RenderingContext()
{
    // If the window is focused, we must be sure to consume all remaining inputs
    // before destroying it. Otherwise, glfwPollEvents might try to read events
    // for an already destroyed window, and lead to a crash.
    // To try to make sure of that, we hide the window, wait a bit and
    // call glfwPollEvents again to catch all events before destroying the window.
    // This might not work on slow systems, umong other situations.
    const auto focused = glfwGetWindowAttrib(_window, GLFW_FOCUSED);
    if (focused)
    {
        glfwHideWindow(_window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        glfwPollEvents();
    }

    if (_window != nullptr)
        glfwDestroyWindow(_window);
}

/*************/
std::unique_ptr<RenderingContext> RenderingContext::createSharedContext(std::string_view name)
{
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    return std::make_unique<RenderingContext>(name, this);
}

/*************/
std::array<int32_t, 4> RenderingContext::getPositionAndSize() const
{
    assert(_window != nullptr);
    std::array<int32_t, 4> posAndSize;

    // Getting window's position is not available on Wayland
    if (!_glfwPlatformWayland)
        glfwGetWindowPos(_window, &posAndSize[0], &posAndSize[1]);

    glfwGetFramebufferSize(_window, &posAndSize[2], &posAndSize[3]);
    return posAndSize;
}

/*************/
void RenderingContext::setPositionAndSize(const std::array<int32_t, 4>& posAndSize)
{
    assert(_window != nullptr);

    // Setting window's position is not available on Wayland
    if (!_glfwPlatformWayland)
        glfwSetWindowPos(_window, posAndSize[0], posAndSize[1]);

    glfwSetWindowSize(_window, posAndSize[2], posAndSize[3]);
}

/*************/
void RenderingContext::setDecorations(bool active)
{
    assert(_window != nullptr);
    assert(_isContextActive);

    const auto posAndSize = getPositionAndSize();

    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, active);
    glfwWindowHint(GLFW_DECORATED, active);
    auto window = glfwCreateWindow(posAndSize[2], posAndSize[3], _name.c_str(), 0, _mainContext->_window);

    // Reset hints to default
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    if (!window)
    {
        Log::get() << Log::WARNING << "RenderingContext::" << __FUNCTION__ << " - Unable to update window " << _name << Log::endl;
        return;
    }

    glfwMakeContextCurrent(window);
    glfwDestroyWindow(_window);
    _window = window;
}

/*************/
void RenderingContext::setAsCurrentContext()
{
    auto currentlyActiveContext = glfwGetCurrentContext();

    if (_isContextActive)
        return;

    assert(_window != nullptr && currentlyActiveContext != _window);
    // A context can only be activated if no other context is active, or if
    // the active context is the main one. This is to prevent too much entanglement.
    if (_mainContext != nullptr && currentlyActiveContext != _mainContext->getGLFWwindow() && currentlyActiveContext != nullptr)
    {
        Log::get() << Log::ERROR << "RenderingContext::" << __FUNCTION__ << " - A rendering context, different from the main one, is already active" << Log::endl;
        assert(false);
        return;
    }

    _previouslyActiveWindow = currentlyActiveContext;
    glfwMakeContextCurrent(_window);
    _isContextActive = true;
}

/*************/
void RenderingContext::releaseContext()
{
    assert(glfwGetCurrentContext() == _window);
    glfwMakeContextCurrent(_previouslyActiveWindow);
    _isContextActive = false;
}

/*************/
void RenderingContext::swapBuffers()
{
    assert(_window != nullptr);
    glfwSwapBuffers(_window);
}

/*************/
void RenderingContext::setCursorVisible(bool visible)
{
    assert(_window != nullptr);
    const auto cursor = visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN;
    if (!_glfwPlatformWayland)
        glfwSetInputMode(_window, GLFW_CURSOR, cursor);
}

/*************/
void RenderingContext::setSwapInterval(int32_t interval)
{
    assert(_window != nullptr);
    assert(_isContextActive);
    glfwSwapInterval(interval);
}

/*************/
void RenderingContext::show()
{
    assert(_window != nullptr);
    assert(_isContextActive);
    glfwShowWindow(_window);
}

/*************/
void RenderingContext::setEventsCallbacks()
{
    glfwSetKeyCallback(_window, Window::keyCallback);
    glfwSetCharCallback(_window, Window::charCallback);
    glfwSetMouseButtonCallback(_window, Window::mouseBtnCallback);
    glfwSetCursorPosCallback(_window, Window::mousePosCallback);
    glfwSetScrollCallback(_window, Window::scrollCallback);
    glfwSetDropCallback(_window, Window::pathdropCallback);
    glfwSetWindowCloseCallback(_window, Window::closeCallback);
}

/*************/
std::vector<std::string> RenderingContext::getMonitorNames() const
{
    std::vector<std::string> names;
    for (const auto& monitor : _monitors)
        names.emplace_back(glfwGetMonitorName(monitor));
    return names;
}

/*************/
std::vector<GLFWmonitor*> RenderingContext::getMonitorList()
{
    int monitorCount = 0;
    const auto glfwMonitors = glfwGetMonitors(&monitorCount);

    if (glfwMonitors == nullptr)
    {
        Log::get() << Log::WARNING << "RenderingContext::" << __FUNCTION__ << " - Unable to retrieve the list of connected monitors" << Log::endl;
        return {};
    }

    std::vector<GLFWmonitor*> monitors;
    for (int i = 0; i < monitorCount; ++i)
    {
        glfwSetMonitorUserPointer(glfwMonitors[i], this);
        monitors.push_back(glfwMonitors[i]);
    }

    return monitors;
}

/*************/
int32_t RenderingContext::getFullscreenMonitor() const
{
    GLFWmonitor* currentMonitor = glfwGetWindowMonitor(_window);
    if (currentMonitor == nullptr)
        return -1;

    auto monitorIt = std::find(_monitors.begin(), _monitors.end(), currentMonitor);
    if (monitorIt == _monitors.end())
    {
        assert(false);
        return -1;
    }

    return static_cast<int32_t>(std::distance(_monitors.begin(), monitorIt));
}

/*************/
bool RenderingContext::setFullscreenMonitor(int32_t index)
{
    if (index >= static_cast<int32_t>(_monitors.size()))
    {
        Log::get() << Log::WARNING << "RenderingContext::" << __FUNCTION__ << " - Monitor index is greater than the monitor count" << Log::endl;
        return false;
    }

    GLFWmonitor* currentMonitor = glfwGetWindowMonitor(_window);

    // If the window is already windowed (not full screen) and the index is -1, do nothing
    if (currentMonitor == nullptr && index == -1)
        return true;

    // If the window is already full screen on the right index, do nothing
    if (index > -1 && currentMonitor == _monitors[index])
        return true;

    // Otherwise, change the full screen state and keep the current monitor resolution
    int workArea[4];
    if (index > -1)
        glfwGetMonitorWorkarea(_monitors[index], &workArea[0], &workArea[1], &workArea[2], &workArea[3]);
    else
        glfwGetMonitorWorkarea(currentMonitor, &workArea[0], &workArea[1], &workArea[2], &workArea[3]);

    if (index == -1)
    {
        // If we passed from full screen to windowed, set the window size to something sensible
        // Namely, center the window and resize it to occupy half the surface

        int newWorkArea[4];
        // Dimensions are divided by roughly sqrt(2), to get a window area of half the monitor area
        newWorkArea[2] = workArea[2] * 100 / 141;
        newWorkArea[3] = workArea[3] * 100 / 141;
        newWorkArea[0] = (workArea[0] - workArea[2]) / 2;
        newWorkArea[1] = (workArea[1] - workArea[3]) / 2;
        glfwSetWindowMonitor(_window, nullptr, newWorkArea[0], newWorkArea[1], newWorkArea[2], newWorkArea[3], GLFW_DONT_CARE);
    }
    else
    {
        glfwSetWindowMonitor(_window, _monitors[index], workArea[0], workArea[1], workArea[2], workArea[3], GLFW_DONT_CARE);
    }

    return true;
}

/*************/
void RenderingContext::glfwMonitorCallback(GLFWmonitor* monitor, int event)
{
    if (event == GLFW_CONNECTED)
        Log::get() << Log::MESSAGE << "RenderingContext::" << __FUNCTION__ << " - Monitor " << glfwGetMonitorName(monitor) << " has been connected" << Log::endl;
    else if (event == GLFW_DISCONNECTED)
        Log::get() << Log::MESSAGE << "RenderingContext::" << __FUNCTION__ << " - Monitor " << glfwGetMonitorName(monitor) << " has been disconnected" << Log::endl;
    else
        assert(false);

    auto that = static_cast<RenderingContext*>(glfwGetMonitorUserPointer(monitor));
    that->_monitors = that->getMonitorList();
}

} // namespace Splash
