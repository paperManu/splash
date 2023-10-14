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

#ifndef SPLASH_RENDERER_H
#define SPLASH_RENDERER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "./core/base_object.h"
#include "./core/graph_object.h"
#include "./utils/log.h"
#include "./graphics/gl_window.h"
#include "./graphics/gpu_buffer.h"

namespace Splash
{

class RootObject;
class Texture_Image;

struct ApiVersion
{
    std::pair<uint, uint> version;
    std::string name;

    std::string toString() const
    {
        const auto major = std::to_string(std::get<0>(version));
        const auto minor = std::to_string(std::get<1>(version));
        return name + " " + major + "." + minor;
    }
};

class Renderer
{
  public:

    enum class Api {
	OpenGL,
	GLES
    };

    static std::shared_ptr<Renderer> fromApi(Renderer::Api api);
    static std::shared_ptr<Renderer> create(std::optional<Renderer::Api> api);

    virtual ~Renderer() = default;

    /**
     *  Callback for GL errors and warnings
     *  TODO: Make private once `Scene::getNewSharedWindow` is moved to the renderer.
     */
    static void glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam); 

    // NOTE: Sometimes you can use `const std::string_view` (pointer + length, trivially and cheaply copied) instead of `const std::string&`, but
    // it is not guaranteed that the view will be null terminated, which we need when interfacing with glfw's C API.
    /**
     *  Initializes glfw, initializes the graphics API, and creates a window.
     */
    void init(const std::string& name);

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
     * Constructor for OpenGLTexture_Image and GLESTexture_Image.
     * \param root Root object
     * \param width Width
     * \param height Height
     * \param pixelFormat String describing the pixel format. Accepted values are RGB, RGBA, sRGBA, RGBA16, R16, YUYV, UYVY, D
     * \param data Pointer to data to use to initialize the texture
     * \param multisample Sample count for MSAA
     * \param cubemap True to request a cubemap
     */
    virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const = 0;
    std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root, int width, int height, const std::string& pixelFormat, int multisample = 0, bool cubemap = false) const;

    virtual std::shared_ptr<GpuBuffer> createGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data) const = 0;

  protected:
    /**
     * Sets API specific flags for OpenGL or OpenGL ES. For example, enables sRGB for OpenGL, or explicitly requests an OpenGL ES context.
     */
    virtual void setApiSpecificFlags() const = 0;

    /**
     * \return A simple struct containing the major and minor OpenGL versions, along with a string name.
     */
    virtual ApiVersion getApiSpecificVersion() const = 0;

    /**
     * \return Calls the appropriate loader for each API. Calls `gladLoadGLES2Loader` for OpenGL ES, and `gladLoadGLLoader`. Note that calling an incorrect loader might lead to
     * segfaults due to API specific function not getting loaded, leaving the pointers as null.
     */
    virtual void loadApiSpecificGlFunctions() const = 0;

  private:
    bool _isInitialized = false;
    std::shared_ptr<GlWindow> _mainWindow;

    std::string _glVendor, _glRenderer;

    /**
     *  Callback for GLFW errors
     * \param code Error code
     * \param msg Associated error message
     */
    static void glfwErrorCallback(int /*code*/, const char* msg)
    {
        Log::get() << Log::ERROR << "glfwErrorCallback - " << msg << Log::endl;
    }

    /**
     * If `api` contains a value, tries creating a window with only the given API, which can fail.
     * If `api` is none, tries OpenGL 4.5, followed by GLES 3.2. This can also fail if the device does not support either.
     * \return a specific renderer implementation if creating a context succeeds, nullptr if it fails.
     */
    static std::shared_ptr<Renderer> findGLVersion(std::optional<Renderer::Api> api);

    /*
     * Loops through a list of predetermined renderers with different APIs.
     * \return The first  renderer that works.
     */
    static std::shared_ptr<Renderer> findCompatibleApi();

    /**
     * Creates a hidden test GLFW window with the given renderer, the renderer sets window flags and hints specific to the API it implements.
     * Can fail if the given renderer's API is not supported. For example: creating an OpenGL 4.5 context on the raspberry pi which doesn't support it.
     * \return true if a context was successfully created, false otherwise.
     */
    static bool tryCreateContext(std::shared_ptr<Renderer> renderer);
};

} // namespace Splash

#endif 
