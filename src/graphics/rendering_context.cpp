#include "./graphics/rendering_context.h"

#include <iostream>

#include <GLFW/glfw3.h>

#include "./graphics/window.h"

namespace Splash
{

bool RenderingContext::_glfwInitialized = false;

/*************/
RenderingContext::RenderingContext(std::string_view name, const gfx::PlatformVersion& platformVersion)
{
    if (!_glfwInitialized)
    {
        glfwSetErrorCallback(glfwErrorCallback);

        if (!glfwInit())
        {
            Log::get() << Log::ERROR << "RenderingContext::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
            return;
        }

        _glfwInitialized = true;
    }

    _name = std::string(name);

    if (platformVersion.name == "OpenGL")
    {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
    }
    else if (platformVersion.name == "OpenGL ES")
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    }

#ifdef DEBUGGL
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
#endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, static_cast<int>(platformVersion.major));
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, static_cast<int>(platformVersion.minor));
    glfwWindowHint(GLFW_VISIBLE, false);
    _window = glfwCreateWindow(512, 512, name.data(), nullptr, nullptr);
}

/*************/
RenderingContext::RenderingContext(std::string_view name, RenderingContext* context)
{
    assert(context != nullptr);
    _name = std::string(name);
    _window = glfwCreateWindow(512, 512, name.data(), nullptr, context->_window);
    _mainContext = context;
}

/*************/
RenderingContext::~RenderingContext()
{
    if (_window != nullptr)
        glfwDestroyWindow(_window);
}

/*************/
std::unique_ptr<RenderingContext> RenderingContext::createSharedContext(std::string_view name)
{
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, false);

    return std::make_unique<RenderingContext>(name, this);
}

/*************/
std::array<int32_t, 4> RenderingContext::getPositionAndSize() const
{
    assert(_window != nullptr);
    std::array<int32_t, 4> posAndSize;
    glfwGetWindowPos(_window, &posAndSize[0], &posAndSize[1]);
    glfwGetFramebufferSize(_window, &posAndSize[2], &posAndSize[3]);
    return posAndSize;
}

/*************/
void RenderingContext::setPositionAndSize(const std::array<int32_t, 4>& posAndSize)
{
    assert(_window != nullptr);
    glfwSetWindowPos(_window, posAndSize[0], posAndSize[1]);
    glfwSetWindowSize(_window, posAndSize[2], posAndSize[3]);
}

/*************/
void RenderingContext::setDecorations(bool active)
{
    assert(_window != nullptr);
    assert(_isContextActive);

    const auto posAndSize = getPositionAndSize();

    glfwWindowHint(GLFW_VISIBLE, true);
    glfwWindowHint(GLFW_RESIZABLE, active);
    glfwWindowHint(GLFW_DECORATED, active);
    auto window = glfwCreateWindow(posAndSize[2], posAndSize[3], ("Splash::" + _name).c_str(), 0, _mainContext->_window);

    // Reset hints to default
    glfwWindowHint(GLFW_RESIZABLE, true);
    glfwWindowHint(GLFW_DECORATED, true);

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

} // namespace Splash
