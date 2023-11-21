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
class Geometry;
class GlWindow;
class RootObject;
class Texture_Image;

namespace gfx
{

class Framebuffer;
class GpuBuffer;

struct ApiVersion
{
    std::pair<uint, uint> version;
    std::string name;

    [[nodiscard]] std::string toString() const
    {
        const auto major = std::to_string(std::get<0>(version));
        const auto minor = std::to_string(std::get<1>(version));
        return name + " " + major + "." + minor;
    }
};

class Renderer
{
  public:
    enum class Api
    {
        OpenGL,
        GLES
    };

    struct GlMsgCallbackData
    {
        std::optional<std::string> name, type;
    };

    static std::unique_ptr<Renderer> fromApi(Renderer::Api api);
    static std::unique_ptr<Renderer> create(std::optional<Renderer::Api> api);

    virtual ~Renderer() = default;

    /**
     *  Callback for GL errors and warnings
     */
    static void glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam);

    // NOTE: Sometimes you can use `const std::string_view` (pointer + length, trivially and cheaply copied) instead of `const std::string&`, but
    // it is not guaranteed that the view will be null terminated, which we need when interfacing with glfw's C API.
    /**
     *  Initializes glfw, initializes the graphics API, and creates a window.
     */
    void init(const std::string& name);

    /**
     * \return A simple struct containing the major and minor OpenGL versions, along with a string name.
     */
    virtual ApiVersion getApiSpecificVersion() const = 0;

    /**
     * \return Returns the version as a pair of {MAJOR, MINOR}
     */
    std::pair<uint, uint> getGLVersion()
    {
        // Shouldn't be here if `version` is `nullopt` (i.e. failed to init).
        return getApiSpecificVersion().version;
    }

    /**
     * \return Returns the vendor of the OpenGL renderer
     */
    std::string getGLVendor() { return _glVendor; }

    /**
     * \return Returns the name of the OpenGL renderer.
     */
    std::string getGLRenderer() { return _glRenderer; }

    /**
     * \return Returns a `shared_ptr` to the main window.
     */
    std::shared_ptr<GlWindow> getMainWindow() { return _mainWindow; }

    /**
     * Get a pointer to the data to be sent to the GL callback
     * \return A raw pointer to the GL callback data
     */
    const Renderer::GlMsgCallbackData* getGlMsgCallbackDataPtr();

    /**
     * Set the user data for the GL callback
     * \param data User data for the callback
     */
    static void setGlMsgCallbackData(const Renderer::GlMsgCallbackData* data) { glDebugMessageCallback(Renderer::glMsgCallback, reinterpret_cast<const void*>(data)); }

    /**
     * Create a new Framebuffer
     * \return Return a shared pointer to the newly created Framebuffer
     */
    virtual std::unique_ptr<gfx::Framebuffer> createFramebuffer() const = 0;

    /**
     * Create a new Geometry
     * \param root Root object
     * \return Return a shared pointer to the newly created Geometry
     */
    virtual std::shared_ptr<Geometry> createGeometry(RootObject* root) const = 0;

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

  private:
    bool _isInitialized = false;
    std::shared_ptr<GlWindow> _mainWindow;
    std::string _glVendor, _glRenderer;

    // Since we don't inherit from BaseObject (because I think it's a bit to heavy for a renderer. (Please ignore my use of virtual functions :P)),
    // we have to re-declare some variable to hold the data we'll pass to the callback in this class.
    Renderer::GlMsgCallbackData _glMsgCallbackData;

    /**
     *  Callback for GLFW errors
     * \param code Error code
     * \param msg Associated error message
     */
    static void glfwErrorCallback(int /*code*/, const char* msg) { Log::get() << Log::ERROR << "glfwErrorCallback - " << msg << Log::endl; }

    /**
     * If `api` contains a value, tries creating a window with only the given API, which can fail.
     * If `api` is none, tries OpenGL 4.5, followed by GLES 3.2. This can also fail if the device does not support either.
     * \return a specific renderer implementation if creating a context succeeds, nullptr if it fails.
     */
    static std::unique_ptr<Renderer> findGLVersion(std::optional<Renderer::Api> api);

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
};

} // namespace gfx
} // namespace Splash

#endif
