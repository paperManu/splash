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
 * @shader_stage.h
 * Shader stage class
 */

#ifndef SPLASH_OPENGL_SHADER_STAGE_H
#define SPLASH_OPENGL_SHADER_STAGE_H

#include <string>

#include "./core/constants.h"
#include "./graphics/api/opengl/gl_utils.h"
#include "./graphics/api/shader_gfx_impl.h"

namespace Splash::gfx::opengl
{

class ShaderStage
{
  public:
    /**
     * Constructor
     */
    explicit ShaderStage(gfx::ShaderType type)
        : _type(type)
    {
    }

    /**
     * Destructor
     */
    ~ShaderStage();

    /**
     * Get the shader id
     * \return Return the shader id
     */
    inline GLuint getId() const { return _shader; }

    /**
     * Get the source code for the shader
     * \return Return the source code
     */
    inline std::string getSource() const { return _source; }

    /**
     * Get the stage type for the shader
     * \return Return the stage type
     */
    inline gfx::ShaderType getType() const { return _type; }

    /**
     * Get whether the shader is valid
     * \return Return true if the shader is valid
     */
    bool isValid() const;

    /**
     * Set the sources for the shader
     * \param source Shader source
     * \return Return true if shader compilation succeeded
     */
    bool setSource(std::string_view source);

    /**
     * Get a string expression of the shader type, used for logging
     * \return Return the shader type as a string
     */
    inline std::string stringFromShaderType() const { return stringFromShaderType(_type); }

    /**
     * Get a string expression of a given shader type, used for logging
     * \param type Shader type
     * \return Return the shader type as a string
     */
    static std::string stringFromShaderType(ShaderType type)
    {
        switch (type)
        {
        default:
            return "";
        case gfx::ShaderType::vertex:
            return "vertex";
        case gfx::ShaderType::tess_ctrl:
            return "tess_ctrl";
        case gfx::ShaderType::tess_eval:
            return "tess_eval";
        case gfx::ShaderType::geometry:
            return "geometry";
        case gfx::ShaderType::fragment:
            return "fragment";
        case gfx::ShaderType::compute:
            return "compute";
        }
    }

    /**
     * Utility functions to get the shader logs. Should only be called on errors.
     * \return Return the error as a string
     */
    std::string getShaderInfoLog() const;

  private:
    GLuint _shader{0};
    gfx::ShaderType _type;
    std::string _source;
};

} // namespace Splash::gfx::opengl

#endif
