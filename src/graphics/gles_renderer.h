#ifndef SPLASH_GLES_RENDERER_H
#define SPLASH_GLES_RENDERER_H

// TODO: Change to an include guard to match splash's coding conventions
#pragma once

#include "./graphics/renderer.h"
#include "./graphics/gles_texture_image.h"

namespace Splash {

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

	virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const final {
	    return std::make_shared<GLESTexture_Image>(root);
	};
    };

}

#endif
