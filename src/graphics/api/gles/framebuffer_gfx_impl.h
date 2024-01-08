/*
 * Copyright (C) 2017 Splash authors
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
 * @framebuffer_gfx_impl.h
 * The framebuffer object class, implemented for OpenGL ES
 */

#ifndef SPLASH_GLES_FRAMEBUFFER_H
#define SPLASH_GLES_FRAMEBUFFER_H

#include "./graphics/api/framebuffer_gfx_impl.h"

namespace Splash::gfx::gles
{

/*************/
class FramebufferGfxImpl final : public Splash::gfx::FramebufferGfxImpl
{
  public:
    /**
     * Constructor
     */
    FramebufferGfxImpl();

    /**
     * Destructor
     */
    ~FramebufferGfxImpl() override;

    /**
     * No copy constructor
     */
    FramebufferGfxImpl(const FramebufferGfxImpl&) = delete;
    FramebufferGfxImpl& operator=(const FramebufferGfxImpl&) = delete;

    /**
     * Bind the FBO to draw into it
     */
    void bindDraw() override;
    void bindRead() override;

    /**
     * Blit the current FBO into the other one
     * \param dst Destination FBO
     */
    void blit(const Splash::gfx::FramebufferGfxImpl* dst) override;

    /**
     * Get the depth at the given location
     * \param x X position
     * \param y Y position
     * \return Return the depth
     */
    float getDepthAt(float x, float y) override;

    /**
     * Get the GL FBO id
     * \return The FBO id
     */
    GLuint getFboId() const override { return _fbo; }

    /**
     * Set the FBO size
     * \param width Width
     * \param height Height
     */
    void setSize(int width, int height) override;

    /**
     * Unbind the FBO
     */
    void unbindDraw() override;
    void unbindRead() override;

  private:
    GLuint _fbo{0};

    /**
     * Set the FBO multisampling and bit per channel settings
     */
    void setRenderingParameters() override;
};

} // namespace Splash::gfx::gles

#endif
