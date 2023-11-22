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
 * Implementation of Camera for OpenGL
 */

#ifndef SPLASH_OPENGL_CAMERA_GFX_IMPL_H
#define SPLASH_OPENGL_CAMERA_GFX_IMPL_H

#include "./core/constants.h"
#include "./graphics/api/camera_gfx_impl.h"

namespace Splash::gfx::opengl
{

class CameraGfxImpl : public gfx::CameraGfxImpl
{
  public:
    /**
     * Constructor
     */
    CameraGfxImpl() = default;

    /**
     * Destructor
     */
    ~CameraGfxImpl() = default;

    /**
     * Activate the give texture for further use
     * \param texId Texture index
     */
    void activateTexture(uint32_t texId) const override final { glActiveTexture(GL_TEXTURE0 + texId); }

    /**
     * Setup the viewport
     * \param width Viewport width
     * \param height Viewport height
     */
    void setupViewport(uint32_t width, uint32_t height) override final
    {
#ifdef DEBUG
        glGetError();
#endif
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        _viewportWidth = width;
        _viewportHeight = height;
    }

    /**
     * Draw a frame of the given color around the viewport
     */
    void beginDrawFrame() const override final
    {
        glClearColor(1.0, 0.5, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        glScissor(_frameWidth, _frameWidth, _viewportWidth - _frameWidth * 2, _viewportHeight - _frameWidth * 2);
    }

    /**
     * Finalize drawing a frame around the viewport
     */
    void endDrawFrame() const override final
    {
        glDisable(GL_SCISSOR_TEST);
    }

    /**
     * Finalize camera rendering
     */
    void endCameraRender() const override final
    {
        glDisable(GL_DEPTH_TEST);
#ifdef DEBUG
        GLenum error = glGetError();
        if (error)
            Log::get() << Log::WARNING << _type << "::" << __FUNCTION__ << " - Error while rendering the camera: " << error << Log::endl;
#endif
    }

    /**
     * Set the clear color for the viewport
     * \param red Red component
     * \param green Green component
     * \param blue Blue component
     * \param alpha Alpha component
     */
    void setClearColor(float red, float green, float blue, float alpha) const override final
    {
        glClearColor(red, green, blue, alpha);
    }

    /**
     * Clear the viewport
     */
    void clearViewport() const override final
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
};

} // namespace Splash::gfx::opengl

#endif
