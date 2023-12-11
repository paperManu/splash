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
 * Base class for renderer, implementation specific rendering API
 */

#ifndef SPLASH_RENDERER_H
#define SPLASH_RENDERER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "./graphics/api/window_gfx_impl.h"
#include "./graphics/gl_window.h"
#include "./utils/log.h"

namespace Splash
{

class BaseObject;
class Filter;
class Geometry;
class GlWindow;
class RootObject;
class Texture_Image;
class Warp;

namespace gfx
{

class CameraGfxImpl;
class FilterGfxImpl;
class Framebuffer;
class GpuBuffer;
class ShaderGfxImpl;

struct PlatformVersion
{
    std::string name;
    uint32_t major{0};
    uint32_t minor{0};
    uint32_t patch{0};

    [[nodiscard]] const std::string toString() const { return name + " " + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); }
};

class Renderer
{
  public:
    enum class Api
    {
        OpenGL,
        GLES
    };

    struct RendererMsgCallbackData
    {
        std::optional<std::string> name, type;
    };

  public:
    /**
     * Create a renderer given a specific API
     * \param api API platform to create a renderer for
     * \return Return a unique pointer to the created renderer
     */
    static std::unique_ptr<Renderer> create(std::optional<Renderer::Api> api);

  private:
    /**
     * Callback for GLFW errors
     * \param code Error code
     * \param msg Associated error message
     */
    static void glfwErrorCallback(int /*code*/, const char* msg) { Log::get() << Log::ERROR << "glfwErrorCallback - " << msg << Log::endl; }

    /**
     * If `api` contains a value, tries creating a window with only the given API, which can fail.
     * If `api` is none, tries OpenGL 4.5, followed by GLES 3.2. This can also fail if the device does not support either.
     * \return a specific renderer implementation if creating a context succeeds, nullptr if it fails.
     */
    static std::unique_ptr<Renderer> findPlatform(std::optional<Renderer::Api> api);

    /*
     * Loops through a list of predetermined renderers with different APIs.
     * \return The first  renderer that works.
     */
    static std::unique_ptr<Renderer> findCompatibleApi();

    /**
     * Creates a hidden test GLFW window with the given renderer, the renderer sets window flags and hints specific to the API it implements.
     * Can fail if the given renderer's API is not supported. For example: creating an OpenGL 4.5 context on the raspberry pi which doesn't support it.
     * \return true if a context was successfully created, false otherwise.
     */
    static bool tryCreateContext(const Renderer* renderer);

  public:
    /**
     * Constructor
     */
    Renderer() = default;

    /**
     * Destructor
     */
    virtual ~Renderer() = default;

    /**
     *  Initialize the renderer
     *  \param name Renderer main window name
     */
    virtual void init(const std::string& name);

    /**
     *  Get a glfw window sharing the same context as _mainWindow
     * \param name Window name
     * \return Return a shared pointer to the new window
     */
    std::shared_ptr<GlWindow> createSharedWindow(const std::string& name = "");

    /**
     * Get whether NV swap groups are available
     * \return Return true if they are
     */
    static bool getHasNVSwapGroup() { return _hasNVSwapGroup; }

    /**
     * Get the platform version
     * \return Returns a PlatformVersion struct
     */
    const PlatformVersion getPlatformVersion() const { return _platformVersion; }

    /**
     * Get the platform vendor
     * \return Returns the vendor of the OpenGL renderer
     */
    std::string getPlatformVendor() { return _platformVendor; }

    /**
     * \return Returns a `shared_ptr` to the main window.
     */
    std::shared_ptr<GlWindow> getMainWindow() { return _mainWindow; }

    /**
     * Set the user data for the GL callback
     * \param data User data for the callback
     */
    virtual void setRendererMsgCallbackData(const Renderer::RendererMsgCallbackData* data) = 0;

    /**
     * Create a new Camera graphics implementation
     * \return Return a unique pointer to a new Camera
     */
    virtual std::unique_ptr<gfx::CameraGfxImpl> createCameraGfxImpl() const = 0;

    /**
     * Create a new Filter graphics implementation
     * \return Return a unique pointer to a new Filter
     */
    virtual std::unique_ptr<gfx::FilterGfxImpl> createFilterGfxImpl() const = 0;

    /**
     * Create a new Framebuffer
     * \return Return a unique pointer to the newly created Framebuffer
     */
    virtual std::unique_ptr<gfx::Framebuffer> createFramebuffer() const = 0;

    /**
     * Create a new Geometry
     * \param root Root object
     * \return Return a shared pointer to the newly created Geometry
     */
    virtual std::shared_ptr<Geometry> createGeometry(RootObject* root) const = 0;

    /**
     * Create a new graphic shader implementation
     * \return Return a unique pointer to the shader implementation
     */
    virtual std::unique_ptr<ShaderGfxImpl> createGraphicShader() const = 0;

    /**
     * Create a new compute shader implementation
     * \return Return a unique pointer to the shader implementation
     */
    virtual std::unique_ptr<ShaderGfxImpl> createComputeShader() const = 0;

    /**
     * Create a new feedback shader implementation
     * \return Return a unique pointer to the shader implementation
     */
    virtual std::unique_ptr<ShaderGfxImpl> createFeedbackShader() const = 0;

    /**
     * Create a new Texture_Image
     * \param root Root object
     * \return Return a shared pointer to a default Texture_Image
     */
    virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const = 0;

    /**
     * Create a new Window
     * \return A class containing the graphics API specific details of `Splash::Window`.
     */
    virtual std::unique_ptr<gfx::WindowGfxImpl> createWindowGfxImpl() const = 0;

  protected:
    /**
     * Sets API specific flags for OpenGL or OpenGL ES. For example, enables sRGB for OpenGL, or explicitly requests an OpenGL ES context.
     */
    virtual void setApiSpecificFlags() const = 0;

    /**
     * \return Calls the appropriate loader for each API. Calls `gladLoadGLES2Loader` for OpenGL ES, and `gladLoadGLLoader`. Note that calling an incorrect loader might lead to
     * segfaults due to API specific function not getting loaded, leaving the pointers as null.
     */
    virtual void loadApiSpecificGlFunctions() const = 0;

    /**
     * Set shared window flags, this is called by createSharedWindow before returning the GlWindow
     */
    virtual void setSharedWindowFlags() const = 0;

  protected:
    static const int _defaultWindowSize{512};
    PlatformVersion _platformVersion;

    std::shared_ptr<GlWindow> _mainWindow;
    std::string _platformVendor;
    std::string _platformRenderer;

    static bool _hasNVSwapGroup; //!< If true, NV swap groups have been detected and are used
    GLuint _maxSwapGroups{0};
    GLuint _maxSwapBarriers{0};

    /**
     * Get a pointer to the data to be sent to the GL callback
     * \return A raw pointer to the GL callback data
     */
    const Renderer::RendererMsgCallbackData* getRendererMsgCallbackDataPtr();

  private:
    Renderer::RendererMsgCallbackData _rendererMsgCallbackData;
    bool _isInitialized = false;
};

} // namespace gfx
} // namespace Splash

#endif
