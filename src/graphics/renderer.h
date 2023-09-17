/*
 * Copyright (C) 2023 Emmanuel Durand
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

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "./core/base_object.h"
#include "./core/graph_object.h"
#include "./utils/log.h"
#include "./graphics/gl_window.h"

namespace Splash
{

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

  protected:
    virtual void setApiSpecificFlags() const = 0;
    virtual ApiVersion getApiSpecificVersion() const = 0;
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

    std::optional<ApiVersion> findGLVersion();
};

class GLESRenderer : public Renderer
{
    virtual void setApiSpecificFlags() const final
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    }

    virtual ApiVersion getApiSpecificVersion() const final 
    {
	return {{3, 2}, "OpenGL ES"};
    }

    virtual void loadApiSpecificGlFunctions() const final 
    { 
	gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress); 
    }
};

class OpenGLRenderer : public Renderer
{
    virtual void setApiSpecificFlags() const final
    {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
    }

    virtual ApiVersion getApiSpecificVersion() const final 
    {
	return {{4, 5}, "OpenGL"};
    }

    virtual void loadApiSpecificGlFunctions() const final 
    { 
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); 
    };
};

static inline std::shared_ptr<Renderer> createRenderer(Renderer::Api api)
{
    std::shared_ptr<Renderer> renderer;
    switch(api) {
	case Renderer::Api::OpenGL: renderer = std::make_shared<GLESRenderer>(); break;
	case Renderer::Api::GLES: renderer = std::make_shared<OpenGLRenderer>(); break;
    }

    // Can't return in the switch, the compiler complains about
    // "control reaches end of non-void function".
    return renderer;
}

} // namespace Splash
