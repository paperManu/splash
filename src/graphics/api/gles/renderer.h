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

#include "./graphics/api/gles/camera_gfx_impl.h"
#include "./graphics/api/gles/compute_shader.h"
#include "./graphics/api/gles/feedback_shader.h"
#include "./graphics/api/gles/filter_gfx_impl.h"
#include "./graphics/api/gles/framebuffer.h"
#include "./graphics/api/gles/geometry_gfx_impl.h"
#include "./graphics/api/gles/gpu_buffer.h"
#include "./graphics/api/gles/graphic_shader.h"
#include "./graphics/api/gles/pbo_gfx_impl.h"
#include "./graphics/api/gles/texture_image_gfx_impl.h"
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
     *  Callback for GL errors and warnings
     */
    static void glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam);

  public:
    /**
     * Constructor
     * Sets the platform version attribute
     */
    Renderer() { _platformVersion = gfx::PlatformVersion({"OpenGL ES", 3, 2, 0}); }

    /**
     *  Initializes the renderer
     *  \param name Renderer main window name
     */
    void init(std::string_view name) override final;

    /**
     * Set the user data for the GL callback
     * \param data User data for the callback
     */
    void setRendererMsgCallbackData(const Renderer::RendererMsgCallbackData* data) override final
    {
        glDebugMessageCallback(Renderer::glMsgCallback, reinterpret_cast<const void*>(data));
    }

    /**
     * Create a new Camera graphics implementation
     * \return Return a unique pointer to a new Camera
     */
    std::unique_ptr<gfx::CameraGfxImpl> createCameraGfxImpl() const override final { return std::make_unique<gfx::gles::CameraGfxImpl>(); }

    /**
     * Create a new Filter graphics implementation
     * \return Return a unique pointer to a new Filter
     */
    std::unique_ptr<gfx::FilterGfxImpl> createFilterGfxImpl() const override final { return std::make_unique<gfx::gles::FilterGfxImpl>(); }

    /**
     * Create a new Framebuffer
     * \return Return a unique pointer to the newly created Framebuffer
     */
    std::unique_ptr<gfx::Framebuffer> createFramebuffer() const override final { return std::make_unique<gles::Framebuffer>(); }

    /**
     * Create a new Geometry
     * \param root Root object
     * \return Return a shared pointer to the newly created Geometry
     */
    std::shared_ptr<Geometry> createGeometry(RootObject* root) const override final { return std::make_shared<Geometry>(root, std::make_unique<gfx::gles::GeometryGfxImpl>()); }

    /**
     * Create a new graphic shader implementation
     * \return Return a unique pointer to the shader implementation
     */
    std::unique_ptr<gfx::ShaderGfxImpl> createGraphicShader() const override final { return std::make_unique<gfx::gles::GraphicShaderGfxImpl>(); }

    /**
     * Create a new compute shader implementation
     * \return Return a unique pointer to the shader implementation
     */
    std::unique_ptr<gfx::ShaderGfxImpl> createComputeShader() const override final { return std::make_unique<gfx::gles::ComputeShaderGfxImpl>(); }

    /**
     * Create a new feedback shader implementation
     * \return Return a unique pointer to the shader implementation
     */
    std::unique_ptr<gfx::ShaderGfxImpl> createFeedbackShader() const override final { return std::make_unique<gfx::gles::FeedbackShaderGfxImpl>(); }

    /**
     * Create a need PBO implementation
     * \param size Underlying PBO count
     * \return Return a unique pointer to the PBO implementation
     */
    std::unique_ptr<gfx::PboGfxImpl> createPboGfxImpl(std::size_t size) const override final { return std::make_unique<gfx::gles::PboGfxImpl>(size); }

    /**
     * Create a new Texture_Image
     * \param root Root object
     * \return Return a shared pointer to a default Texture_Image
     */
    std::unique_ptr<gfx::Texture_ImageGfxImpl> createTexture_ImageGfxImpl() const override final { return std::make_unique<gfx::gles::Texture_ImageGfxImpl>(); }

    /**
     * Create a new Window
     * \return A class containing the graphics API specific details of `Splash::Window`.
     */
    std::unique_ptr<gfx::WindowGfxImpl> createWindowGfxImpl() const override final { return std::make_unique<gfx::gles::WindowGfxImpl>(); }

  private:
    /**
     * Calls the appropriate loader for each API. Calls `gladLoadGLES2Loader` for OpenGL ES, and `gladLoadGLLoader`. Note that calling an incorrect loader might lead to
     * segfaults due to API specific function not getting loaded, leaving the pointers as null.
     */
    void loadApiSpecificFunctions() const override final { gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress); }

    /**
     * Set shared window flags, this is called by createSharedContext before returning the RenderingContext
     */
    void setSharedWindowFlags() const final override;
};

} // namespace Splash::gfx::gles

#endif
