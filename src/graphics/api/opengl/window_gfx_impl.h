#include "./graphics/api/gl_base_window_gfx_impl.h"

namespace Splash::gfx::opengl
{

class WindowGfxImpl : public gfx::GlBaseWindowGfxImpl
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
    virtual void swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, int _windowRect[4]) override final
    {
        _window->setAsCurrentContext();
        glWaitSync(_renderFence, 0, GL_TIMEOUT_IGNORED);

        // If this is the first window to be swapped, or NVSwapGroups are active,
        // this window should be synchronized to the vertical sync. So we will draw to back buffer
        const bool isWindowSynchronized = Scene::getHasNVSwapGroup() or windowIndex == 0;

        // If the window is not synchronized, draw directly to front buffer
        if (!isWindowSynchronized)
            glDrawBuffer(GL_FRONT);

        if (_srgb)
            glEnable(GL_FRAMEBUFFER_SRGB);

        // If the render texture specs have changed
        if (_renderTextureUpdated)
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
            _renderTextureUpdated = false;
        }

        // Copy the rendered texture to the back/front buffer
        glBlitNamedFramebuffer(_readFbo, 0, 0, 0, _windowRect[2], _windowRect[3], 0, 0, _windowRect[2], _windowRect[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);

        if (_srgb)
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
};

} // namespace Splash::gfx::opengl
