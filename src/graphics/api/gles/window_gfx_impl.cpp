#include "./graphics/api/gles/window_gfx_impl.h"

#include "./core/root_object.h"
#include "./core/scene.h"
#include "./graphics/texture_image.h"

namespace Splash::gfx::gles
{

/*************/
WindowGfxImpl::~WindowGfxImpl()
{
    glDeleteFramebuffers(1, &_renderFbo);
    glDeleteFramebuffers(1, &_readFbo);
}

/*************/
void WindowGfxImpl::setupFBOs(Scene* scene, int windowRect[4])
{
    // Render FBO
    if (glIsFramebuffer(_renderFbo) == GL_FALSE)
    {
        glGenFramebuffers(1, &_renderFbo);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);

    _depthTexture = scene->getRenderer()->createTexture_Image(scene, windowRect[2], windowRect[3], "D");
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);

    _colorTexture = scene->getRenderer()->createTexture_Image(scene);
    _colorTexture->setAttribute("filtering", {false});
    _colorTexture->reset(windowRect[2], windowRect[3], "RGBA");
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);

    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, fboBuffers);

    const auto _status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::WARNING << "Window::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << _status << Log::endl;
#ifdef DEBUG
    else
        Log::get() << Log::DEBUGGING << "Window::" << __FUNCTION__ << " - Render framebuffer object successfully initialized" << Log::endl;
#endif

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/*************/
void WindowGfxImpl::clearScreen(glm::vec4 color, bool clearDepth)
{
    glClearColor(color[0], color[1], color[2], color[3]);

    uint flags = GL_COLOR_BUFFER_BIT;

    if (clearDepth)
        flags |= GL_DEPTH_BUFFER_BIT;

    glClear(flags);
}

/*************/
void WindowGfxImpl::resetSyncFence()
{
    glDeleteSync(_renderFence);
    _renderFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

/*************/
void WindowGfxImpl::beginRender(int windowRect[4])
{
    glViewport(0, 0, windowRect[2], windowRect[3]);

#ifdef DEBUG
    glGetError();
#endif

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);
}

/*************/
void WindowGfxImpl::endRender()
{
#ifdef DEBUG
    GLenum error = glGetError();
    if (error)
        Log::get() << Log::WARNING << "WindowGfxImpl"
                   << "::" << __FUNCTION__ << " - Error while rendering the window: " << error << Log::endl;
#endif

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/*************/
void WindowGfxImpl::setDebugData(const void* userData)
{
    glDebugMessageCallback(gfx::Renderer::glMsgCallback, userData);
}

/*************/
void WindowGfxImpl::init(Scene* scene)
{
    auto w = scene->getNewSharedWindow();
    if (w.get() == nullptr)
        return;
    _window = w;
}

/*************/
void WindowGfxImpl::swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, int _windowRect[4])
{
    // OpenGL ES doesn't seem to support rendering directly to the frontbuffer, so instead, we render to the back buffer and swap.
    //
    // This implementation causes the framerate to halve for each window we add onto splash on nvidia GPUs _with vsync on_. If you have one window for the GUI and another for
    // the projector view(like the default config), the FPS will be half of the refresh rate (if you can hit it). Each window will decrease FPS further. This is due to either a bug
    // caused by OpenGL ES's limitations or a bug in the driver.

    _window->setAsCurrentContext();
    glWaitSync(_renderFence, 0, GL_TIMEOUT_IGNORED);

    // If this is the first window to be swapped, or NVSwapGroups are active,
    // this window should be synchronized to the vertical sync. So we will draw to back buffer
    const bool isWindowSynchronized = Scene::getHasNVSwapGroup() or windowIndex == 0;

    // If the window is not synchronized, draw directly to front buffer
    if (!isWindowSynchronized)
    {
        // Since we can't draw to the front buffer directly in OpenGL ES,
        // we draw to the back buffer, then immediately present it
        GLenum buffers[] = {GL_BACK};
        glDrawBuffers(1, buffers);
        glfwSwapBuffers(_window->get());
    }

    if (_srgb)
        glEnable(GL_FRAMEBUFFER_SRGB);

    // If the render texture specs have changed
    if (_renderTextureUpdated)
    {
        if (_readFbo != 0)
            glDeleteFramebuffers(1, &_readFbo);

        glGenFramebuffers(1, &_readFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _readFbo);

        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);
        const auto status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            Log::get() << Log::WARNING << "gfx::gles::WindowGfxImpl::" << __FUNCTION__ << " - Error while initializing read framebuffer object: " << status << Log::endl;
#ifdef DEBUG
        else
            Log::get() << Log::DEBUGGING << "gfx::gles::WindowGfxImpl::" << __FUNCTION__ << " - Read framebuffer object successfully initialized" << Log::endl;
#endif
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        _renderTextureUpdated = false;
    }

    // Copy the rendered texture to the back/front buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _readFbo);
    glBlitFramebuffer(0, 0, _windowRect[2], _windowRect[3], 0, 0, _windowRect[2], _windowRect[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    if (_srgb)
        glDisable(GL_FRAMEBUFFER_SRGB);

    if (isWindowSynchronized)
    {
        // If this window is synchronized, so we wait for the vsync and swap
        // front and back buffers
        glfwSwapBuffers(_window->get());
    }
    else
    {
        // If this window is not synchronized, we revert the draw buffer back
        // to drawing to the back buffer
        GLenum buffers[] = {GL_BACK};
        glDrawBuffers(1, buffers);
    }

    _window->releaseContext();
}

} // namespace Splash::gfx::gles
