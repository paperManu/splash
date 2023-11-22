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
 * @camera_gfx_impl.h
 * Base class for Camera API implementations
 */

#ifndef SPLASH_CAMERA_GFX_IMPL_H
#define SPLASH_CAMERA_GFX_IMPL_H

#include "./core/constants.h"

namespace Splash::gfx
{

class CameraGfxImpl
{
  public:
    /**
     * Constructor
     */
    CameraGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~CameraGfxImpl() = default;

    /**
     * Activate the give texture for further use
     * \param texId Texture index
     */
    virtual void activateTexture(uint32_t texId) const = 0;

    /**
     * Setup the viewport
     * \param width Viewport width
     * \param height Viewport height
     */
    virtual void setupViewport(uint32_t width, uint32_t height) = 0;

    /**
     * Draw a frame of the given color around the viewport
     */
    virtual void beginDrawFrame() const = 0;

    /**
     * Finalize drawing a frame around the viewport
     */
    virtual void endDrawFrame() const = 0;

    /**
     * Finalize camera rendering
     */
    virtual void endCameraRender() const = 0;

    /**
     * Set the clear color for the viewport
     * \param red Red component
     * \param green Green component
     * \param blue Blue component
     * \param alpha Alpha component
     */
    virtual void setClearColor(float red, float green, float blue, float alpha) const = 0;

    /**
     * Clear the viewport
     */
    virtual void clearViewport() const = 0;

  protected:
    static constexpr int _frameWidth{8};
    uint32_t _viewportWidth{0};
    uint32_t _viewportHeight{0};
};

} // namespace Splash::gfx

#endif
