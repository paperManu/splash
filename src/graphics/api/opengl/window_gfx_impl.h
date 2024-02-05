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
 * Implementation of WindowGfx for OpenGL
 */

#ifndef SPLASH_GFX_GL_WINDOW_IMPL_H
#define SPLASH_GFX_GL_WINDOW_IMPL_H

#include <memory>

#include "./core/constants.h"
#include "./graphics/api/window_gfx_impl.h"
#include "./graphics/rendering_context.h"

namespace Splash
{

class Texture_Image;

namespace gfx::opengl
{

class WindowGfxImpl : public Splash::gfx::WindowGfxImpl
{
  public:
    /**
     * Constructor
     */
    WindowGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~WindowGfxImpl() override;

    /**
     * Setup the FBOs for this window
     * \param scene Pointer to the Scene
     * \param width FBO width
     * \param height FBO height
     */
    void setupFBOs(Scene* scene, uint32_t width, uint32_t height) final;

    /**
     * Clear the screen with the given color
     * \param color Clear color
     * \param clearDepth If true, also clears the depth buffer
     */
    void clearScreen(glm::vec4 color, bool clearDepth = false) final;

    /**
     * Reset the render sync fence
     */
    void resetSyncFence() final;

    /**
     * Setup the window for beginning rendering
     * \param width Width of the rendering viewport
     * \param height Height of the rendering viewport
     */
    void beginRender(uint32_t width, uint32_t height) final;

    /**
     * End rendering for this window
     */
    void endRender() final;

    /**
     * Initialize the window
     * \param renderer Pointer to the renderer
     */
    void init(Renderer* renderer) final;

    /**
     * Set as current rendering context
     */
    inline void setAsCurrentContext() final { _renderingContext->setAsCurrentContext(); }

    /**
     * Release from begin the current rendering context
     */
    inline void releaseContext() final { _renderingContext->releaseContext(); }

    /**
     * Check whether this window is the current rendering context
     * \return Return true if this window is the current rendering context
     */
    inline bool isCurrentContext() const final { return _renderingContext->isCurrentContext(); }

    /**
     * Check whether the GLFW window exists
     * \return Return true if the contained GLFW window exists
     */
    bool windowExists() const final { return _renderingContext != nullptr; }

    /**
     * Swap back and front buffers
     * \param windowIndex Window index, among all of Splash windows
     * \param renderTextureUpdated True if the render textures have been updated
     * \param width Width of the rendering viewport
     * \param height Height of the rendering viewport
     */
    void swapBuffers(int windowIndex, bool& renderTextureUpdated, uint32_t width, uint32_t height) final;

  private:
    GLuint _readFbo{0};
    GLuint _renderFbo{0};

    GLsync _renderFence{nullptr};

    std::shared_ptr<Texture_Image> _depthTexture{nullptr};
    std::shared_ptr<Texture_Image> _colorTexture{nullptr};
};

} // namespace gfx::opengl

} // namespace Splash

#endif
