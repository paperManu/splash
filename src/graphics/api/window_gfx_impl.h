/*
 * Copyright (C) 2023 Splash authors
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @window_gfx_impl.h
 * Base class for window API implementations
 */

#ifndef SPLASH_GFX_WINDOW_GFX_IMPL
#define SPLASH_GFX_WINDOW_GFX_IMPL

#include <glm/vec4.hpp>

#include "./graphics/rendering_context.h"

struct GLFWwindow;

namespace Splash
{

class Scene;
class RootObject;

namespace gfx
{

class Renderer;

class WindowGfxImpl
{
  public:
    /**
     * Constructor
     */
    WindowGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~WindowGfxImpl() = default;

    /**
     * Setup the FBOs for this window
     * \param scene Pointer to the Scene
     * \param width FBO width
     * \param height FBO height
     */
    virtual void setupFBOs(Scene* _scene, uint32_t width, uint32_t height) = 0;

    /**
     * Clear the screen with the given color
     * \param color Clear color
     * \param clearDepth If true, also clears the depth buffer
     */
    virtual void clearScreen(glm::vec4 color, bool clearDepth = false) = 0;

    /**
     * Reset the render sync fence
     */
    virtual void resetSyncFence() = 0;

    /**
     * Setup the window for beginning rendering
     * \param width Width of the rendering viewport
     * \param height Height of the rendering viewport
     */
    virtual void beginRender(uint32_t width, uint32_t height) = 0;

    /**
     * End rendering for this window
     */
    virtual void endRender() = 0;

    /**
     * Get the GLFW window
     * \return Return a pointer to the GLFW window
     */
    inline RenderingContext* getRenderingContext() { return _renderingContext.get(); }

    /**
     * Get the main rendering context
     * \return Return a pointer to the  main rendering context
     */
    inline RenderingContext* getMainContext() { return _renderingContext->getMainContext(); }

    /**
     * Initialize the window
     * \param renderer Pointer to the renderer
     */
    virtual void init(Renderer* renderer) = 0;

    /**
     * Set as current rendering context
     */
    virtual inline void setAsCurrentContext() = 0;

    /**
     * Release from begin the current rendering context
     */
    virtual inline void releaseContext() = 0;

    /**
     * Check whether this window is the current rendering context
     * \return Return true if this window is the current rendering context
     */
    virtual inline bool isCurrentContext() const = 0;

    /**
     * Check whether the GLFW window exists
     * \return Return true if the contained GLFW window exists
     */
    virtual bool windowExists() const = 0;

    /**
     * Copy the given portion of rendered FBO up to the front buffer, respecting vertical sync if active
     * \param windowIndex Window index, among all of Splash windows
     * \param srgb True if the window is rendered in the sRGB color space
     * \param renderTextureUpdated True if the render textures have been updated
     * \param width Width of the rendering viewport
     * \param height Height of the rendering viewport
     */
    virtual void swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, uint32_t width, uint32_t height) = 0;

  protected:
    std::unique_ptr<RenderingContext> _renderingContext{nullptr};
};

} // namespace gfx

} // namespace Splash

#endif
