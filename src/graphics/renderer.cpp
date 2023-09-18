#include "./graphics/renderer.h"

#include "./graphics/opengl_renderer.h"
#include "./graphics/gles_renderer.h"


namespace Splash {

    std::shared_ptr<Renderer> Renderer::create(Renderer::Api api)
    {
	std::shared_ptr<Renderer> renderer;
	switch(api) {
	    case Renderer::Api::GLES: renderer = std::make_shared<GLESRenderer>(); break;
	    case Renderer::Api::OpenGL: renderer = std::make_shared<OpenGLRenderer>(); break;
	}

	// Can't return in the switch, the compiler complains about
	// "control reaches end of non-void function".
	return renderer;
    }

    void Renderer::glMsgCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* userParam)
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

    void Renderer::init(const std::string& name)
    {
        glfwSetErrorCallback(glfwErrorCallback);

        // GLFW stuff
        if (!glfwInit())
        {
            Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to initialize GLFW" << Log::endl;
            _isInitialized = false;
            return;
        }

        const auto glVersion = findGLVersion();
        if (!glVersion)
        {
            Log::get() << Log::ERROR << "Scene::" << __FUNCTION__ << " - Unable to find a suitable GL version (higher than 4.3)" << Log::endl;
            _isInitialized = false;
            return;
        }

        Log::get() << Log::MESSAGE << "Renderer::" << __FUNCTION__ << " - GL version: " << glVersion.value().toString() << Log::endl;

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

    std::optional<ApiVersion> Renderer::findGLVersion()
    {
        setApiSpecificFlags();

	auto apiVersion = getApiSpecificVersion();

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


    std::shared_ptr<Texture_Image> Renderer::createTexture_Image(RootObject* root, int width, int height, const std::string& pixelFormat, const GLvoid* data, int multisample, bool cubemap) const {
	auto tex = createTexture_Image(root);
	tex->reset(width, height, pixelFormat, data, multisample, cubemap);

	return tex;
    }
};
