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
 * @feedback_shader.h
 * Class for feedback shader, implementated for OpenGL
 */

#ifndef SPLASH_OPENGL_FEEDBACK_SHADER_GFX_IMPL_H
#define SPLASH_OPENGL_FEEDBACK_SHADER_GFX_IMPL_H

#include <string>
#include <vector>

#include "./core/constants.h"
#include "./graphics/api/feedback_shader.h"
#include "./graphics/api/opengl/shader_gfx_impl.h"

namespace Splash::gfx::opengl
{

class FeedbackShaderGfxImpl : public gfx::FeedbackShaderGfxImpl, public ShaderGfxImpl
{
  public:
    /**
     * Constructor
     */
    FeedbackShaderGfxImpl();

    /**
     * Destructor
     */
    virtual ~FeedbackShaderGfxImpl() override = default;

    /**
     * Activate the shader
     * \return Return true if the activation succeeded
     */
    bool activate() override final;

    /**
     * Deactivate the program
     */
    void deactivate() override final;

    /**
     * Select the outputs (varyings) for the feedback
     * \param varyingNames Names of the outputs
     */
    void selectVaryings(const std::vector<std::string>& varyingNames) override final;
};

} // namespace Splash::gfx::opengl

#endif
