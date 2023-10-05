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

#ifndef SPLASH_GLES_RENDERER_H
#define SPLASH_GLES_RENDERER_H

#include "./graphics/renderer.h"
#include "./graphics/gles_texture_image.h"
#include "./graphics/gles_gpu_buffer.h"

namespace Splash {

    class GLESRenderer : public Renderer
    {
	virtual void setApiSpecificFlags() const override final
	{
	    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	}

	virtual ApiVersion getApiSpecificVersion() const override final 
	{
	    return {{3, 2}, "OpenGL ES"};
	}

	virtual void loadApiSpecificGlFunctions() const override final 
	{ 
	    gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress); 
	}

	virtual std::shared_ptr<Texture_Image> createTexture_Image(RootObject* root) const override final {
	    return std::make_shared<GLESTexture_Image>(root);
	};

	virtual std::shared_ptr<GpuBuffer> createGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data) const override final 
	{
	    return std::make_shared<GLESGpuBuffer>(elementSize, type, usage, size, data);
	}
    };

}

#endif
