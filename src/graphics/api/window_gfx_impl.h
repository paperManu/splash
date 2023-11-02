#ifndef SPLASH_GFX_WINDOW_GFX_IMPL
#define SPLASH_GFX_WINDOW_GFX_IMPL

#include "glm/vec4.hpp"

struct GLFWwindow;

namespace Splash
{

class Scene;
class RootObject;

namespace gfx
{

class WindowGfxImpl
{
  public:
    virtual ~WindowGfxImpl() = default;

    virtual void setupFBOs(Scene* _scene, int _windowRect[4]) = 0;

    virtual void clearScreen(glm::vec4 color, bool clearDepth = false) = 0;

    virtual void resetSyncFence() = 0;

    virtual void beginRender(int _windowRect[4]) = 0;

    virtual void endRender() = 0;

    virtual void setDebugData(const void* userData) = 0;

    virtual inline GLFWwindow* getGlfwWindow() = 0;

    virtual inline GLFWwindow* getMainWindow() = 0;

    virtual void init(Scene* scene) = 0;

    virtual inline void setAsCurrentContext() = 0;

    virtual inline void releaseContext() = 0;

    virtual inline bool isCurrentContext() const = 0;

    virtual void updateGlfwWindow(GLFWwindow* window) = 0;

    // Hmmm, contextExists might be a better name?
    virtual bool windowExists() const = 0;

    virtual void swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, int _windowRect[4]) = 0;
};

} // namespace gfx

} // namespace Splash

#endif
