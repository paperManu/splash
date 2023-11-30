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
 * @shader_gfx_impl.h
 * Class for shader, implementated for OpenGL ES
 */

#ifndef SPLASH_GLES_SHADER_GFX_IMPL_H
#define SPLASH_GLES_SHADER_GFX_IMPL_H

#include <map>
#include <string>
#include <string_view>

#include "./core/constants.h"
#include "./graphics/api/gles/shader_program.h"
#include "./graphics/api/gles/shader_stage.h"
#include "./graphics/api/shader_gfx_impl.h"

namespace Splash::gfx::gles
{

class ShaderGfxImpl : virtual public gfx::ShaderGfxImpl
{
  public:
    /**
     * Constructor
     */
    ShaderGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~ShaderGfxImpl() = default;

    /**
     * Activate the shader
     * \return Return true if the activation succeeded
     */
    virtual bool activate() override;

    /**
     * Deactivate the program
     */
    virtual void deactivate() override;

    /**
     * Get a map of all uniforms and their values
     * \return Return the uniforms and their values
     */
    virtual std::map<std::string, Values> getUniformValues() const override final;

    /**
     * Get the documentation for the uniforms based on the comments in GLSL code
     * \return Return a map of uniforms and their documentation
     */
    virtual std::map<std::string, std::string> getUniformsDocumentation() const override final;

    /**
     * Set the source for the given shader stage type
     * \param type Shader stage type
     * \param source Shader source
     * \return Return true if it succeeded
     */
    virtual bool setSource(gfx::ShaderType type, std::string_view source) override final;

    /**
     * Set a given uniform for the activated shader
     * \param name Uniform name
     * \param value Uniform value
     */
    virtual void setUniform(const std::string& name, const Value& value) override { _program->setUniform(name, value); }

    /**
     * Reset the given shader type
     * \param stage Shader type to reset
     */
    virtual void removeShaderType(gfx::ShaderType type) override final;

  protected:
    std::unique_ptr<Program> _program{nullptr};
    std::map<gfx::ShaderType, ShaderStage> _shaderStages;
};

} // namespace Splash::gfx::gles

#endif
