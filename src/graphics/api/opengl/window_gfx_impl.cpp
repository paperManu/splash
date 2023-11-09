#include "./graphics/api/opengl/window_gfx_impl.h"

#include "./core/root_object.h"
#include "./core/scene.h"
#include "./graphics/texture_image.h"

namespace Splash::gfx::opengl
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
    glDebugMessageCallback(Renderer::glMsgCallback, userData);
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
void WindowGfxImpl::swapBuffers(int windowIndex, bool srgb, bool& renderTextureUpdated, int windowRect[4])
{
    /* Back to front buffer swap is done a bit differently than what is usually done
     * in most software. To understand why and how it works, you have to know that
     * with Xorg (which is for now the main target for the use of Splash), vertical
     * synchronization is done for each OpenGL context separately even when contexts
     * are shared. So for each context, the OpenGL driver has to wait for the vsync
     * to happen before being authorized to go forward.
     *
     * Splash can be configured to render to multiple windows, to multiple displays.
     * In this case what can happen is that one window blocks due to waiting for the
     * buffer swap, which may lead to missing the swap for the next window. Which will
     * then wait for the next one, and so on. In this context, all displays (screens
     * or projectors) are considered to have the same refresh rate, and ideally to
     * be synchronized.
     *
     * This means that buffer swapping has to be done manually. The following
     * swapBuffers() method is called for each window sequentially (in Scene::render()),
     * and overall here is what happens:
     * - the first window renders to the back buffer, and waits for the vertical sync
     * - all subsequent windows draw directly to the front buffer
     *
     * As the drawing in this method is merely a buffer copy, it is fast enough for
     * the subsequent windows to be rendered soon enough for the rendering to front
     * buffer to be invisible (as in, not producing any visible glitch). And the whole
     * rendering is then synchronized only once for all of Splash.
     *
     * Note that in the case where NVIDIA Quadro and Quadro sync cards are detected,
     * and that NVSwapGroups are available, all of this behavior is mostly disabled as
     * NVIDIA drivers take care of vertical synchronization correctly. So vertical
     * synchronization happens as usual in this case.
     */

    _window->setAsCurrentContext();
    glWaitSync(_renderFence, 0, GL_TIMEOUT_IGNORED);

    // If this is the first window to be swapped, or NVSwapGroups are active,
    // this window should be synchronized to the vertical sync. So we will draw to back buffer
    const bool isWindowSynchronized = Scene::getHasNVSwapGroup() or windowIndex == 0;

    // If the window is not synchronized, draw directly to front buffer
    if (!isWindowSynchronized)
        glDrawBuffer(GL_FRONT);

    if (srgb)
        glEnable(GL_FRAMEBUFFER_SRGB);

    // If the render texture specs have changed
    if (renderTextureUpdated)
    {
        if (_readFbo != 0)
            glDeleteFramebuffers(1, &_readFbo);

        glCreateFramebuffers(1, &_readFbo);

        glNamedFramebufferTexture(_readFbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
        const auto status = glCheckNamedFramebufferStatus(_readFbo, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            Log::get() << Log::WARNING << "gfx::opengl::WindowGfxImpl::" << __FUNCTION__ << " - Error while initializing read framebuffer object: " << status << Log::endl;
#ifdef DEBUG
        else
            Log::get() << Log::DEBUGGING << "gfx::opengl::WindowGfxImpl::" << __FUNCTION__ << " - Read framebuffer object successfully initialized" << Log::endl;
#endif
        renderTextureUpdated = false;
    }

    // Copy the rendered texture to the back/front buffer
    glBlitNamedFramebuffer(_readFbo, 0, 0, 0, windowRect[2], windowRect[3], 0, 0, windowRect[2], windowRect[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);

    if (srgb)
        glDisable(GL_FRAMEBUFFER_SRGB);

    if (isWindowSynchronized)
        // If this window is synchronized, so we wait for the vsync and swap
        // front and back buffers
        glfwSwapBuffers(_window->get());
    else
        // If this window is not synchronized, we revert the draw buffer back
        // to drawing to the back buffer
        glDrawBuffer(GL_BACK);

    _window->releaseContext();
}

} // namespace Splash::gfx::opengl
