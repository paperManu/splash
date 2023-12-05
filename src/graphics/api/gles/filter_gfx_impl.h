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
 * Implementation of FilterGfxImpl for OpenGL ES
 */

#ifndef SPLASH_GLES_FILTER_GFX_IMPL_H
#define SPLASH_GLES_FILTER_GFX_IMPL_H

#include "./graphics/api/filter_gfx_impl.h"

namespace Splash::gfx::gles
{

class FilterGfxImpl : public gfx::FilterGfxImpl
{
  public:
    /**
     * Constructor
     */
    FilterGfxImpl() = default;

    /**
     * Destructor
     */
    ~FilterGfxImpl() = default;

    /**
     * Setup the viewport for the filter
     * \param width Viewport width
     * \param height Viewport height
     */
    void setupViewport(uint32_t width, uint32_t height) const override final
    {
        glViewport(0, 0, width, height);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    /**
     * Enable multisampling
     */
    void enableMultisampling() const override final { glEnable(GL_MULTISAMPLE); }

    /**
     * Disable multisampling
     */
    void disableMultisampling() const override final { glDisable(GL_MULTISAMPLE); }

    /**
     * Enable rendering to and from cubemap textures
     */
    void enableCubemapRendering() const override final { glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); }

    /**
     * Disable rendering to and from cubemap textures
     */
    void disableCubemapRendering() const override final { glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS); }
};

} // namespace Splash::gfx::gles

#endif
