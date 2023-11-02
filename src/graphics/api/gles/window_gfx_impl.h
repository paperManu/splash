#include "./graphics/api/gl_base_window_gfx_impl.h"
namespace Splash::gfx::gles
{

class WindowGfxImpl : public gfx::GlBaseWindowGfxImpl
{
    // OpenGL ES doesn't seem to support rendering directly to the frontbuffer, so instead, we render to the back buffer and swap.
    //
    // TODO: This implementation causes the framerate to halve for each window we add onto splash on nvidia GPUs _with vsync on_. If you have one window for the GUI and another for
    // the projector view(like the default config), the FPS will be half of the refresh rate (if you can hit it). Each window will decrease FPS further. This is due to either a bug
    // caused by OpenGL ES's limitations or a bug in the driver.
    virtual void swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, int _windowRect[4]) override final
    {
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
};

} // namespace Splash::gfx::gles
