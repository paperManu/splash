#include "./core/base_object.h"
#include "./core/graph_object.h"
#include "./utils/log.h"
#include "graphics/gl_window.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

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
    /**
     *  Callback for GL errors and warnings
     *  TODO: Make private once `Scene::getNewSharedWindow` is moved to the renderer.
     */
    static void glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam)
    {
        const auto userParamsAsObj = reinterpret_cast<const BaseObject*>(userParam);
        std::string typeString{"GL::other"};
        std::string messageString;
        Log::Priority logType{Log::MESSAGE};

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            typeString = "GL::Error";
            logType = Log::ERROR;
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeString = "GL::Deprecated behavior";
            logType = Log::WARNING;
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeString = "GL::Undefined behavior";
            logType = Log::ERROR;
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typeString = "GL::Portability";
            logType = Log::WARNING;
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typeString = "GL::Performance";
            logType = Log::WARNING;
            break;
        case GL_DEBUG_TYPE_OTHER:
            typeString = "GL::Other";
            logType = Log::MESSAGE;
            break;
        }

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_LOW:
            messageString = typeString + "::low";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            messageString = typeString + "::medium";
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            messageString = typeString + "::high";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            // Disable notifications, they are far too verbose
            return;
            // messageString = "\033[32;1m[" + typeString + "::notification]\033[0m";
            // break;
        }

        if (userParam == nullptr)
            Log::get() << logType << messageString << " - Object: unknown"
                       << " - " << message << Log::endl;
        else if (const auto userParamsAsGraphObject = dynamic_cast<const GraphObject*>(userParamsAsObj); userParamsAsGraphObject != nullptr)
            Log::get() << logType << messageString << " - Object " << userParamsAsGraphObject->getName() << " of type " << userParamsAsGraphObject->getType() << " - " << message
                       << Log::endl;
        else
            Log::get() << logType << messageString << " - Object " << userParamsAsObj->getName() << " - " << message << Log::endl;
    }

    // NOTE: Sometimes you can use `const std::string_view` (pointer + length, trivially and cheaply copied) instead of `const std::string&`, but
    // it is not guaranteed that the view will be null terminated, which we need when interfacing with glfw's C API.
    void init(const std::string& name)
    {
        glfwSetErrorCallback(glfwErrorCallback);

        // GLFW stuff
        if (!glfwInit())
        {
            Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
            _isInitialized = false;
            return;
        }

        _glVersion = findGLVersion();
        if (!_glVersion)
        {
            Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to find a suitable GL version (higher than 4.3)" << Log::endl;
            _isInitialized = false;
            return;
        }

        const auto glVersion = _glVersion.value();
        Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - GL version: " << glVersion.toString() << Log::endl;

#ifdef DEBUGGL
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#else
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
#endif

        // Should already have most flags set by `findGLVersion`.
        GLFWwindow* window = glfwCreateWindow(512, 512, name.c_str(), NULL, NULL);

        if (!window)
        {
            Log::get() << Log::WARNING << "Scene::" << __FUNCTION__ << " - Unable to create a GLFW window" << Log::endl;
            _isInitialized = false;
            return;
        }

        _mainWindow = std::make_shared<GlWindow>(window, window);
        _isInitialized = true;

        _mainWindow->setAsCurrentContext();

        loadApiSpecificGlFunctions();

        // Get hardware information
        _glVendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        _glRenderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL vendor: " << _glVendor << Log::endl;
        Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - GL renderer: " << _glRenderer << Log::endl;

// Activate GL debug messages
#ifdef DEBUGGL
        glDebugMessageCallback(glMsgCallback, reinterpret_cast<void*>(this));
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
#endif

// Check for swap groups
#ifdef GLX_NV_swap_group
        if (glfwExtensionSupported("GLX_NV_swap_group"))
        {
            PFNGLXQUERYMAXSWAPGROUPSNVPROC nvGLQueryMaxSwapGroups = (PFNGLXQUERYMAXSWAPGROUPSNVPROC)glfwGetProcAddress("glXQueryMaxSwapGroupsNV");
            if (!nvGLQueryMaxSwapGroups(glfwGetX11Display(), 0, &_maxSwapGroups, &_maxSwapBarriers))
                Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - Unable to get NV max swap groups / barriers" << Log::endl;
            else
                Log::get() << Log::MESSAGE << "Scene::" << __FUNCTION__ << " - NV max swap groups: " << _maxSwapGroups << " / barriers: " << _maxSwapBarriers << Log::endl;

            if (_maxSwapGroups != 0)
                _hasNVSwapGroup = true;
        }
#endif
        _mainWindow->releaseContext();
    }

    /**
     * Get the found OpenGL version
     * \return Return the version as a pair of {MAJOR, MINOR}
     */
    std::pair<uint, uint> getGLVersion()
    {
        // Shouldn't be here if `version` is `nullopt` (i.e. failed to init).
        return _glVersion.value().version;
    }

    /**
     * Get the vendor of the OpenGL renderer
     * \return Return the vendor of the OpenGL renderer
     */
    std::string getGLVendor()
    {
        return _glVendor;
    }

    /**
     * Get the name of the OpenGL renderer
     * \return Return the name of the OpenGL renderer
     */
    std::string getGLRenderer()
    {
        return _glRenderer;
    }

    std::shared_ptr<GlWindow> mainWindow()
    {
        return _mainWindow;
    }

  private:
    bool _isInitialized = false;
    std::optional<ApiVersion> _glVersion;
    std::shared_ptr<GlWindow> _mainWindow;

    std::string _glVendor, _glRenderer;

    virtual ApiVersion apiSpecificFlags() = 0;
    virtual void loadApiSpecificGlFunctions() = 0;

    /**
     *  Callback for GLFW errors
     * \param code Error code
     * \param msg Associated error message
     */
    static void glfwErrorCallback(int /*code*/, const char* msg)
    {
        Log::get() << Log::WARNING << "glfwErrorCallback - " << msg << Log::endl;
    }

    std::optional<ApiVersion> findGLVersion()
    {
        auto apiVersion = apiSpecificFlags();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, std::get<0>(apiVersion.version));
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, std::get<1>(apiVersion.version));
        glfwWindowHint(GLFW_VISIBLE, false);

        GLFWwindow* window = glfwCreateWindow(512, 512, "test_window", NULL, NULL);

        if (window)
        {
            glfwDestroyWindow(window);
            return apiVersion;
        }

        return {};
    }
};

class GLESRenderer : public Renderer
{
    virtual ApiVersion apiSpecificFlags() override
    {
        std::pair<uint, uint> versionToCheck = {3, 2};
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

        return {versionToCheck, "GLES"};
    }

    virtual void loadApiSpecificGlFunctions() override { gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress); }
};

class OpenGLRenderer : public Renderer
{
    virtual ApiVersion apiSpecificFlags() override
    {
        std::pair<uint, uint> versionToCheck = {4, 5};

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        return {versionToCheck, "OpenGL"};
    }

    virtual void loadApiSpecificGlFunctions() override { gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); };
};

static inline std::shared_ptr<Renderer> createRenderer(bool gles)
{
    if (gles)
    {
        return std::make_shared<GLESRenderer>();
    }
    else
    {
        return std::make_shared<OpenGLRenderer>();
    }
}

} // namespace Splash
