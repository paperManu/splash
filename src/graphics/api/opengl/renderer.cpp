#include "./graphics/api/opengl/renderer.h"

#include "./core/constants.h"

namespace Splash::gfx::opengl
{

/*************/
void Renderer::glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam)
{
    const auto callbackData = reinterpret_cast<const Renderer::RendererMsgCallbackData*>(userParam);

    std::string typeString{"GL::other"};
    std::string messageString;
    Log::Priority logType{Log::MESSAGE};

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        typeString = "GL::Error";
        logType = Log::ERROR;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeString = "GL::Deprecated behavior";
        logType = Log::WARNING;
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeString = "GL::Undefined behavior";
        logType = Log::ERROR;
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeString = "GL::Portability";
        logType = Log::WARNING;
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeString = "GL::Performance";
        logType = Log::WARNING;
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeString = "GL::Other";
        logType = Log::MESSAGE;
        break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_LOW:
        messageString = typeString + "::low";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        messageString = typeString + "::medium";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        messageString = typeString + "::high";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        // Disable notifications, they are far too verbose
        return;
    }

    if (callbackData == nullptr)
        Log::get() << logType << messageString << " - Object: unknown"
                   << " - " << message << Log::endl;
    else if (callbackData->name && callbackData->type)
        Log::get() << logType << messageString << " - Object " << callbackData->name.value() << " of type " << callbackData->type.value() << " - " << message << Log::endl;
    else if (callbackData->name)
        Log::get() << logType << messageString << " - Object " << callbackData->name.value() << " - " << message << Log::endl;
}

/*************/
void Renderer::init(const std::string& name)
{
    gfx::Renderer::init(name);

    _mainWindow->setAsCurrentContext();

    // Get hardware information
    _platformVendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    _platformRenderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - GL vendor: " << _platformVendor << Log::endl;
    Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - GL renderer: " << _platformRenderer << Log::endl;

// Activate GL debug messages
#ifdef DEBUGGL
    Renderer::setGlMsgCallbackData(getGlMsgCallbackDataPtr());
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif

#ifdef GLX_NV_swap_group
    // Check for swap groups
    if (glfwExtensionSupported("GLX_NV_swap_group"))
    {
        auto nvGLQueryMaxSwapGroups = static_cast<PFNGLXQUERYMAXSWAPGROUPSNVPROC>(glfwGetProcAddress("glXQueryMaxSwapGroupsNV"));
        if (!nvGLQueryMaxSwapGroups(glfwGetX11Display(), 0, &_maxSwapGroups, &_maxSwapBarriers))
            Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - Unable to get NV max swap groups / barriers" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - NV max swap groups: " << _maxSwapGroups << " / barriers: " << _maxSwapBarriers << Log::endl;

        if (_maxSwapGroups != 0)
            _hasNVSwapGroup = true;
    }
#endif

    _mainWindow->releaseContext();
}

/*************/
void Renderer::setSharedWindowFlags() const
{
#ifdef DEBUGGL
    setRendererMsgCallbackData(getRendererMsgCallbackDataPtr());
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif

#ifdef GLX_NV_swap_group
    if (_maxSwapGroups)
    {
        PFNGLXJOINSWAPGROUPNVPROC nvGLJoinSwapGroup = (PFNGLXJOINSWAPGROUPNVPROC)glfwGetProcAddress("glXJoinSwapGroupNV");
        auto nvResult = nvGLJoinSwapGroup(glfwGetX11Display(), glfwGetGLXWindow(window), 1);
        if (nvResult)
            Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - Window " << windowName << " successfully joined the NV swap group" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - Window " << windowName << " couldn't join the NV swap group" << Log::endl;
    }

    if (_maxSwapBarriers)
    {
        PFNGLXBINDSWAPBARRIERNVPROC nvGLBindSwapBarrier = (PFNGLXBINDSWAPBARRIERNVPROC)glfwGetProcAddress("glXBindSwapBarrierNV");
        auto nvResult = nvGLBindSwapBarrier(glfwGetX11Display(), 1, 1);
        if (nvResult)
            Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - Window " << windowName << " successfully bind the NV swap barrier" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - Window " << windowName << " couldn't bind the NV swap barrier" << Log::endl;
    }
#endif
}

} // namespace Splash::gfx::opengl
