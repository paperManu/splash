#ifndef SPLASH_OPENGL_RENDERER_H
#define SPLASH_OPENGL_RENDERER_H

#include "./graphics/renderer.h"
#include "./graphics/opengl_texture_image.h"

namespace Splash {
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

	virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const final {
	    return std::make_shared<OpenGLTexture_Image>(root);
	};
    };
}

#endif
