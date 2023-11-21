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
 * @renderer.h
 * Implementation of Renderer for OpenGL ES
 */

#ifndef SPLASH_GLES_RENDERER_H
#define SPLASH_GLES_RENDERER_H

#include "./graphics/api/gles/framebuffer.h"
#include "./graphics/api/gles/geometry_gfx_impl.h"
#include "./graphics/api/gles/gpu_buffer.h"
#include "./graphics/api/gles/texture_image_gfx_impl.h"
#include "./graphics/api/gles/warp_gfx_impl.h"
#include "./graphics/api/gles/window_gfx_impl.h"
#include "./graphics/api/renderer.h"
#include "./graphics/texture_image.h"
#include "./graphics/warp.h"

namespace Splash::gfx::gles
{

class Renderer : public gfx::Renderer
{
  public:
    /**
     * \return A simple struct containing the major and minor OpenGL versions, along with a string name.
     */
    virtual gfx::ApiVersion getApiSpecificVersion() const override final { return {{3, 2}, "OpenGL ES"}; }

  private:
    /**
     * Sets API specific flags for OpenGL or OpenGL ES. For example, enables sRGB for OpenGL, or explicitly requests an OpenGL ES context.
     */
    virtual void setApiSpecificFlags() const override final { glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API); }

    /**
     * Calls the appropriate loader for each API. Calls `gladLoadGLES2Loader` for OpenGL ES, and `gladLoadGLLoader`. Note that calling an incorrect loader might lead to
     * segfaults due to API specific function not getting loaded, leaving the pointers as null.
     */
    virtual void loadApiSpecificGlFunctions() const override final { gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress); }

    /**
     * Create a new Framebuffer
     * \return Return a shared pointer to the newly created Framebuffer
     */
    virtual std::unique_ptr<gfx::Framebuffer> createFramebuffer() const override final { return std::make_unique<gles::Framebuffer>(); }

    /**
     * Create a new Geometry
     * \param root Root object
     * \return Return a shared pointer to the newly created Geometry
     */
    virtual std::shared_ptr<Geometry> createGeometry(RootObject* root) const override final
    {
        return std::make_shared<Geometry>(root, std::make_unique<gfx::gles::GeometryGfxImpl>());
    }

    /**
     * Create a new Texture_Image
     * \param root Root object
     * \return Return a shared pointer to a default Texture_Image
     */
    virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const override final
    {
        return std::make_shared<Texture_Image>(root, std::make_unique<gfx::gles::Texture_ImageGfxImpl>());
    };

    /**
     * Create a Warp
     * \param root Root object
     * \return Return a shared pointer to a new Warp
     */
    virtual std::shared_ptr<Warp> createWarp(RootObject* root) const override final { return std::make_shared<Warp>(root, std::make_unique<gfx::gles::WarpGfxImpl>()); }

    /**
     * Create a new Window
     * \return A class containing the graphics API specific details of `Splash::Window`.
     */
    virtual std::unique_ptr<gfx::WindowGfxImpl> createWindowGfxImpl() const override final { return std::make_unique<gfx::gles::WindowGfxImpl>(); }
};

} // namespace Splash

#endif
