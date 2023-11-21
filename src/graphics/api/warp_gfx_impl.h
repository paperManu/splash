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
 * @warp_gfx_impl.h
 * Base class for Warp API implementations
 */

#ifndef SPLASH_WARP_GFX_IMPL_H
#define SPLASH_WARP_GFX_IMPL_H

#include "./core/constants.h"

namespace Splash::gfx
{

class WarpGfxImpl
{
  public:
    /**
     * Constructor
     */
    WarpGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~WarpGfxImpl() = default;

    /**
     * Setup the viewport for the warp
     * \param width Viewport width
     * \param height Viewport height
     */
    virtual void setupViewport(uint32_t width, uint32_t height) = 0;
};

} // namespace Splash::gfx

#endif
