#ifndef SPLASH_GLES_RENDERER_H
#define SPLASH_GLES_RENDERER_H

#include "./graphics/renderer.h"
#include "./graphics/gles_texture_image.h"
#include "graphics/gles_gpu_buffer.h"

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

	virtual std::shared_ptr<GpuBuffer> createGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data) const override final 
	{
	    return std::make_shared<GLESGpuBuffer>(elementSize, type, usage, size, data);
	}
    };

}

#endif
