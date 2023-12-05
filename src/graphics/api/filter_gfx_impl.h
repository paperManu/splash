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
 * @filter_gfx_impl.h
 * Base class for Filter API implementations
 */

#ifndef SPLASH_FILTER_GFX_IMPL_H
#define SPLASH_FILTER_GFX_IMPL_H

#include "./core/constants.h"

namespace Splash::gfx
{

class FilterGfxImpl
{
  public:
    /**
     * Constructor
     */
    FilterGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~FilterGfxImpl() = default;

    /**
     * Setup the viewport for the filter
     * \param width Viewport width
     * \param height Viewport height
     */
    virtual void setupViewport(uint32_t width, uint32_t height) const = 0;

    /**
     * Enable multisampling
     */
    virtual void enableMultisampling() const = 0;

    /**
     * Disable multisampling
     */
    virtual void disableMultisampling() const = 0;

    /**
     * Enable rendering to and from cubemap textures
     */
    virtual void enableCubemapRendering() const = 0;

    /**
     * Disable rendering to and from cubemap textures
     */
    virtual void disableCubemapRendering() const = 0;
};

} // namespace Splash::gfx

#endif
