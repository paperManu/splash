#include "./graphics/api/renderer.h"

#include "./core/base_object.h"
#include "./core/graph_object.h"
#include "./graphics/api/gles/renderer.h"
#include "./graphics/api/gpu_buffer.h"
#include "./graphics/api/opengl/renderer.h"
#include "./graphics/rendering_context.h"
#include "./graphics/texture_image.h"

namespace Splash::gfx
{

std::string Renderer::_platformVendor{};
std::string Renderer::_platformRenderer{};
bool Renderer::_hasNVSwapGroup{false};

/*************/
std::unique_ptr<Renderer> Renderer::create(std::optional<Renderer::Api> api)
{
    auto renderer = findPlatform(api);
    if (!renderer)
    {
        Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to find a suitable GL version (OpenGL 4.5 or OpenGL ES 3.2)" << Log::endl;
        return nullptr;
    }

    const auto platformVersion = renderer->getPlatformVersion().toString();
    Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - GL version: " << platformVersion << Log::endl;

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
    const auto platformVersion = renderer->getPlatformVersion();
    auto renderingContext = RenderingContext("test_window", platformVersion);

    return renderingContext.isInitialized();
}

/*************/
void Renderer::init(std::string_view name)
{
    // Should already have most flags set by `findPlatform`.
    _mainRenderingContext = std::make_unique<RenderingContext>(name, _platformVersion);
    if (!_mainRenderingContext->isInitialized())
        return;

    _mainRenderingContext->setAsCurrentContext();
    loadApiSpecificFunctions();
    _mainRenderingContext->releaseContext();
}

/*************/
std::unique_ptr<RenderingContext> Renderer::createSharedContext(std::string_view name)
{
    assert(_mainRenderingContext != nullptr);
    const std::string windowName = name.empty() ? "Splash::Window" : "Splash::" + std::string(name);

    auto sharedRenderingContext = _mainRenderingContext->createSharedContext(windowName);
    if (sharedRenderingContext == nullptr)
    {
        Log::get() << Log::WARNING << "Renderer::" << __FUNCTION__ << " - Unable to create new shared context" << Log::endl;
        return {};
    }

    sharedRenderingContext->setAsCurrentContext();
    setSharedWindowFlags();
    sharedRenderingContext->releaseContext();

    return sharedRenderingContext;
}

/*************/
const Renderer::RendererMsgCallbackData* Renderer::getRendererMsgCallbackDataPtr()
{
    _rendererMsgCallbackData.name = getPlatformVersion().toString();
    _rendererMsgCallbackData.type = "Renderer";
    return &_rendererMsgCallbackData;
}

}; // namespace Splash::gfx
