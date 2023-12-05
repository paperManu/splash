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
 * Base class for shader, implementation specific rendering API
 */

#ifndef SPLASH_SHADER_GFX_IMPL_H
#define SPLASH_SHADER_GFX_IMPL_H

#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

#include <glm/gtc/type_ptr.hpp>

#include "./core/constants.h"
#include "./core/value.h"

namespace Splash::gfx
{

enum class ShaderType : GLenum
{
    vertex = GL_VERTEX_SHADER,
    tess_ctrl = GL_TESS_CONTROL_SHADER,
    tess_eval = GL_TESS_EVALUATION_SHADER,
    geometry = GL_GEOMETRY_SHADER,
    fragment = GL_FRAGMENT_SHADER,
    compute = GL_COMPUTE_SHADER
};

enum class ProgramType : uint8_t
{
    Graphic = 0,
    Compute,
    Feedback
};

enum class Culling : uint8_t
{
    doubleSided = 0,
    singleSided,
    inverted
};

class ShaderGfxImpl
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
    virtual bool activate() = 0;

    /**
     * Deactivate the program
     */
    virtual void deactivate() = 0;

    /**
     * Get a map of all uniforms and their values
     * \return Return the uniforms and their values
     */
    virtual std::map<std::string, Values> getUniformValues() const = 0;

    /**
     * Get the documentation for the uniforms based on the comments in GLSL code
     * \return Return a map of uniforms and their documentation
     */
    virtual std::map<std::string, std::string> getUniformsDocumentation() const = 0;

    /**
     * Set the source for the given shader stage type
     * \param type Shader stage type
     * \param source Shader source
     * \return Return true if it succeeded
     */
    virtual bool setSource(ShaderType type, std::string_view source) = 0;

    /**
     * Set a given uniform for the activated shader
     * \param name Uniform name
     * \param value Uniform value
     */
    virtual void setUniform(const std::string& name, const Values& value) = 0;

    /**
     * Reset the given shader type
     * \param stage Shader type to reset
     */
    virtual void removeShaderType(ShaderType type) = 0;

  protected:
    bool _isActive{false};
};

} // namespace Splash::gfx

#endif
