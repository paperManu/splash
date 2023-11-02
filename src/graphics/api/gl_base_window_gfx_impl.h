#ifndef SPLASH_GFX_GL_BASE_WINDOW_GFX_IMPL
#define SPLASH_GFX_GL_BASE_WINDOW_GFX_IMPL

#include "./core/root_object.h"
#include "./core/scene.h"
#include "./graphics/api/window_gfx_impl.h"
#include "./graphics/texture_image.h"

namespace Splash::gfx
{

class GlBaseWindowGfxImpl : public gfx::WindowGfxImpl
{
  public:
    virtual ~GlBaseWindowGfxImpl()
    {
        glDeleteFramebuffers(1, &_renderFbo);
        glDeleteFramebuffers(1, &_readFbo);
    }

    virtual void setupFBOs(Scene* _scene, int _windowRect[4]) override final
    {
        // Render FBO
        if (glIsFramebuffer(_renderFbo) == GL_FALSE)
        {
            glGenFramebuffers(1, &_renderFbo);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);

        _depthTexture = _scene->getRenderer()->createTexture_Image(_scene, _windowRect[2], _windowRect[3], "D");
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);

        _colorTexture = _scene->getRenderer()->createTexture_Image(_scene);
        _colorTexture->setAttribute("filtering", {false});
        _colorTexture->reset(_windowRect[2], _windowRect[3], "RGBA");
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

    void clearScreen(glm::vec4 color, bool clearDepth = false) override final
    {
        glClearColor(color[0], color[1], color[2], color[3]);

        uint flags = GL_COLOR_BUFFER_BIT;

        if (clearDepth)
            flags |= GL_DEPTH_BUFFER_BIT;

        glClear(flags);
    }

    void resetSyncFence() override final
    {
        glDeleteSync(_renderFence);
        _renderFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    void beginRender(int _windowRect[4]) override final
    {
        glViewport(0, 0, _windowRect[2], _windowRect[3]);

#ifdef DEBUG
        glGetError();
#endif

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFbo);
    }

    void endRender() override final
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

    void setDebugData(const void* userData) override final
    {
        glDebugMessageCallback(Renderer::glMsgCallback, userData);
    }

    inline GLFWwindow* getGlfwWindow() override final
    {
        return _window->get();
    }

    inline GLFWwindow* getMainWindow() override final
    {
        return _window->getMainWindow();
    }

    void init(Scene* scene) override final
    {
        auto w = scene->getNewSharedWindow();
        if (w.get() == nullptr)
            return;
        _window = w;
    }

    inline void setAsCurrentContext() override final
    {
        _window->setAsCurrentContext();
    }

    inline void releaseContext() override final
    {
        _window->releaseContext();
    }

    inline bool isCurrentContext() const override final
    {
        return _window->isCurrentContext();
    }

    void updateGlfwWindow(GLFWwindow* window) override final
    {
        _window = std::make_shared<GlWindow>(window, _window->getMainWindow());
    }

    bool windowExists() const override final
    {
        return _window != nullptr;
    }

  protected:
    virtual void swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, int _windowRect[4]) override = 0;

  protected:
    GLuint _readFbo{0};
    GLuint _renderFbo{0};

    std::shared_ptr<GlWindow> _window{nullptr};

    GLsync _renderFence{nullptr};

    std::shared_ptr<Texture_Image> _depthTexture{nullptr};
    std::shared_ptr<Texture_Image> _colorTexture{nullptr};
};

} // namespace Splash::gfx

#endif
