#include "./graphics/api/renderer.h"

#include "./core/base_object.h"
#include "./core/graph_object.h"
#include "./graphics/api/gles/renderer.h"
#include "./graphics/api/gpu_buffer.h"
#include "./graphics/api/opengl/renderer.h"
#include "./graphics/gl_window.h"
#include "./graphics/texture_image.h"

namespace Splash::gfx
{

/*************/
std::unique_ptr<Renderer> Renderer::create(std::optional<Renderer::Api> api)
{
    glfwSetErrorCallback(glfwErrorCallback);

    // GLFW stuff
    if (!glfwInit())
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
        return nullptr;
    }

    auto renderer = findGLVersion(api);
    if (!renderer)
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to find a suitable GL version (OpenGL 4.5 or OpenGL ES 3.2)" << Log::endl;
        return nullptr;
    }

    const auto platformVersion = renderer->getPlatformVersion().toString();
    Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - GL version: " << platformVersion << Log::endl;

#ifdef DEBUGGL
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
#endif

    return renderer;
}

/*************/
std::unique_ptr<Renderer> Renderer::findGLVersion(std::optional<Renderer::Api> api)
{
    if (api)
    {
        std::unique_ptr<Renderer> renderer{nullptr};

        switch (api.value())
        {
        default:
            assert(false);
            break;
        case Renderer::Api::GLES:
            renderer = std::make_unique<gles::Renderer>();
            break;
        case Renderer::Api::OpenGL:
            renderer = std::make_unique<opengl::Renderer>();
            break;
        }

        if (tryCreateContext(renderer.get()))
            return renderer;
        else
            return {nullptr};
    }
    else
    {
        return findCompatibleApi();
    }
}

/*************/
std::unique_ptr<Renderer> Renderer::findCompatibleApi()
{
    Log::get() << Log::MESSAGE << "No rendering API specified, will try finding a compatible one" << Log::endl;

    if (auto renderer = std::make_unique<opengl::Renderer>(); tryCreateContext(renderer.get()))
    {
        Log::get() << Log::MESSAGE << "Context " << renderer->getPlatformVersion().toString() << " created successfully!" << Log::endl;
        return renderer;
    }
    else if (auto renderer = std::make_unique<gles::Renderer>(); tryCreateContext(renderer.get()))
    {
        Log::get() << Log::MESSAGE << "Context " << renderer->getPlatformVersion().toString() << " created successfully!" << Log::endl;
        return renderer;
    }

    Log::get() << Log::MESSAGE << "Failed to create a context with any rendering API!" << Log::endl;
    return {nullptr};
}

/*************/
bool Renderer::tryCreateContext(const Renderer* renderer)
{
    renderer->setApiSpecificFlags();

    const auto platformVersion = renderer->getPlatformVersion();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, platformVersion.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, platformVersion.minor);
    glfwWindowHint(GLFW_VISIBLE, false);

    GLFWwindow* window = glfwCreateWindow(512, 512, "test_window", NULL, NULL);

    if (window)
    {
        glfwDestroyWindow(window);
        return true;
    }

    return false;
}

/*************/
void Renderer::glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam)
{
    const auto callbackData = reinterpret_cast<const Renderer::GlMsgCallbackData*>(userParam);

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
        // messageString = "\033[32;1m[" + typeString + "::notification]\033[0m";
        // break;
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
    // Should already have most flags set by `findGLVersion`.
    GLFWwindow* window = glfwCreateWindow(512, 512, name.c_str(), NULL, NULL);

    if (!window)
    {
        Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
        _isInitialized = false;
        return;
    }

    _mainWindow = std::make_shared<GlWindow>(window, window);
    _isInitialized = true;

    _mainWindow->setAsCurrentContext();

    loadApiSpecificGlFunctions();

    // Get hardware information
    _glVendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    _glRenderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL vendor: " << _glVendor << Log::endl;
    Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL renderer: " << _glRenderer << Log::endl;

// Activate GL debug messages
#ifdef DEBUGGL
    Renderer::setGlMsgCallbackData(getGlMsgCallbackDataPtr());
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif

// Check for swap groups
#ifdef GLX_NV_swap_group
    if (glfwExtensionSupported("GLX_NV_swap_group"))
    {
        PFNGLXQUERYMAXSWAPGROUPSNVPROC nvGLQueryMaxSwapGroups = (PFNGLXQUERYMAXSWAPGROUPSNVPROC)glfwGetProcAddress("glXQueryMaxSwapGroupsNV");
        if (!nvGLQueryMaxSwapGroups(glfwGetX11Display(), 0, &_maxSwapGroups, &_maxSwapBarriers))
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Unable to get NV max swap groups / barriers" << Log::endl;
        else
            Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - NV max swap groups: " << _maxSwapGroups << " / barriers: " << _maxSwapBarriers << Log::endl;

        if (_maxSwapGroups != 0)
            _hasNVSwapGroup = true;
    }
#endif
    _mainWindow->releaseContext();
}

/*************/
const Renderer::GlMsgCallbackData* Renderer::getGlMsgCallbackDataPtr()
{
    _glMsgCallbackData.name = getPlatformVersion().toString();
    _glMsgCallbackData.type = "Renderer";
    return &_glMsgCallbackData;
}

}; // namespace Splash::gfx
