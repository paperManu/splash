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
 * Implementation of Renderer for OpenGL
 */

#ifndef SPLASH_OPENGL_RENDERER_H
#define SPLASH_OPENGL_RENDERER_H

#include "./graphics/api/opengl/geometry_gfx_impl.h"
#include "./graphics/api/opengl/gpu_buffer.h"
#include "./graphics/api/opengl/texture_image_gfx_impl.h"
#include "./graphics/api/opengl/window_gfx_impl.h"
#include "./graphics/api/renderer.h"

namespace Splash::gfx::opengl
{

class Renderer : public gfx::Renderer
{
  public:
    virtual gfx::ApiVersion getApiSpecificVersion() const override final { return {{4, 5}, "OpenGL"}; }

  private:
    virtual void setApiSpecificFlags() const override final
    {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
    }

    virtual void loadApiSpecificGlFunctions() const override final { gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); };

    virtual std::shared_ptr<Geometry> createGeometry(RootObject* root) const override final
    {
        return std::make_shared<Geometry>(root, std::make_unique<gfx::opengl::GeometryGfxImpl>());
    }

    virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const override final
    {
        return std::make_shared<Texture_Image>(root, std::make_unique<gfx::opengl::Texture_ImageGfxImpl>());
    };

    virtual std::unique_ptr<gfx::WindowGfxImpl> createWindowGfxImpl() const override final { return std::make_unique<gfx::opengl::WindowGfxImpl>(); }
};

} // namespace Splash

#endif
