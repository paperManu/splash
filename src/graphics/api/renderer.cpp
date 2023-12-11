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

bool Renderer::_hasNVSwapGroup{false};

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

    auto renderer = findPlatform(api);
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
std::unique_ptr<Renderer> Renderer::findPlatform(std::optional<Renderer::Api> api)
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

    GLFWwindow* window = glfwCreateWindow(_defaultWindowSize, _defaultWindowSize, "test_window", NULL, NULL);

    if (window)
    {
        glfwDestroyWindow(window);
        return true;
    }

    return false;
}

/*************/
void Renderer::init(const std::string& name)
{
    // Should already have most flags set by `findPlatform`.
    GLFWwindow* window = glfwCreateWindow(_defaultWindowSize, _defaultWindowSize, name.c_str(), NULL, NULL);

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
    _mainWindow->releaseContext();
}

/*************/
std::shared_ptr<GlWindow> Renderer::createSharedWindow(const std::string& name)
{
    const std::string windowName = name.empty() ? "Splash::Window" : "Splash::" + name;

    if (!_mainWindow)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Main window does not exist, unable to create new shared window" << Log::endl;
        return {nullptr};
    }

    // The GL version is the same as in the initialization, so we don't have to reset it here
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(_defaultWindowSize, _defaultWindowSize, windowName.c_str(), NULL, _mainWindow->get());
    if (!window)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Unable to create new shared window" << Log::endl;
        return {nullptr};
    }
    auto glWindow = std::make_shared<GlWindow>(window, _mainWindow->get());

    glWindow->setAsCurrentContext();
    setSharedWindowFlags();
    glWindow->releaseContext();

    return glWindow;
}

/*************/
const Renderer::RendererMsgCallbackData* Renderer::getRendererMsgCallbackDataPtr()
{
    _rendererMsgCallbackData.name = getPlatformVersion().toString();
    _rendererMsgCallbackData.type = "Renderer";
    return &_rendererMsgCallbackData;
}

}; // namespace Splash::gfx
