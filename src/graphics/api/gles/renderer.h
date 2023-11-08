/*
 * Copyright (C) 2023 Tarek Yasser
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

#ifndef SPLASH_GLES_RENDERER_H
#define SPLASH_GLES_RENDERER_H

#include "./graphics/api/gles/geometry_gfx_impl.h"
#include "./graphics/api/gles/texture_image_impl.h"
#include "./graphics/api/gles/window_gfx_impl.h"
#include "./graphics/api/gles/gpu_buffer.h"
#include "./graphics/api/renderer.h"

namespace Splash::gfx::gles
{

class Renderer : public gfx::Renderer
{
  public:
    virtual gfx::ApiVersion getApiSpecificVersion() const override final { return {{3, 2}, "OpenGL ES"}; }

  private:
    virtual void setApiSpecificFlags() const override final { glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API); }

    virtual void loadApiSpecificGlFunctions() const override final { gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress); }

    virtual std::shared_ptr<Geometry> createGeometry(RootObject* root) const override final
    {
        return std::make_shared<Geometry>(root, std::make_unique<gfx::gles::GeometryGfxImpl>());
    }

    virtual std::shared_ptr<gfx::GpuBuffer> createGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data) const override final
    {
        return std::make_shared<gfx::gles::GpuBuffer>(elementSize, type, usage, size, data);
    }

    virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const override final
    {
        return std::make_shared<Texture_Image>(root, std::make_unique<gfx::gles::Texture_ImageImpl>());
    };

    virtual std::unique_ptr<gfx::WindowGfxImpl> createWindowGfxImpl() const override final { return std::make_unique<gfx::gles::WindowGfxImpl>(); }
};

} // namespace Splash

#endif
