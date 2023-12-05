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
 * @shader_program.h
 * Shader program class
 */

#ifndef SPLASH_OPENGL_SHADER_PROGRAM_H
#define SPLASH_OPENGL_SHADER_PROGRAM_H

#include <map>
#include <string>
#include <string_view>

#include <glm/gtc/type_ptr.hpp>

#include "./core/constants.h"
#include "./core/value.h"
#include "./graphics/api/opengl/gl_utils.h"
#include "./graphics/api/opengl/shader_stage.h"
#include "./graphics/api/shader_gfx_impl.h"
#include "./graphics/texture.h"

namespace Splash::gfx::opengl
{

class Program
{
  public:
    struct Uniform
    {
        std::string type{""};
        uint32_t elementSize{1};
        uint32_t arraySize{0};
        Values values{};
        GLint glIndex{-1};
        GLuint glBuffer{0};
        bool glBufferReady{false};
    };

  public:
    /**
     * Constructor
     */
    explicit Program(ProgramType type);

    /**
     * Destructor
     */
    ~Program();

    /**
     * Activate the program
     * \return Return true if the activation succeeded
     */
    bool activate();

    /**
     * Deactivate the program
     */
    void deactivate();

    /**
     * Attach a shader stage to the program
     * \param stage Shader stage
     */
    void attachShaderStage(const ShaderStage& stage);

    /**
     * Detach a shader type from the program
     * \param type Shader type
     */
    void detachShaderStage(gfx::ShaderType type);

    /**
     * Utility functions to get the program logs. Should only be called on errors.
     * \return Return the program logs as a string
     */
    std::string getProgramInfoLog() const;

    /**
     * Get the program type
     * \return Return the program type
     */
    ProgramType getType() const { return _type; }

    /**
     * Get a map of all uniforms and their values
     * \return Return the uniforms and their values
     */
    const std::map<std::string, Values> getUniformValues() const;

    /**
     * Get a reference the the program uniforms
     * \return Return a reference to the uniforms
     */
    inline const std::map<std::string, Uniform>& getUniforms() const { return _uniforms; }

    /**
     * Get the documentation for the uniforms based on the comments in GLSL code
     * \return Return a map of uniforms and their documentation
     */
    std::map<std::string, std::string> getUniformsDocumentation() const { return _uniformsDocumentation; }

    /**
     * Get whether the program is linked
     * \return Return true if the program is linked
     */
    inline bool isLinked() const { return _isLinked; }

    /**
     * Get whether the program is valid and compiled
     * \return Return true if the program is usable
     */
    bool isValid() const;

    /**
     * Link the shader program
     * \return Return true if the linking succeeded
     */
    bool link();

    /**
     * Set the given uniform
     * \param name Uniform name
     * \param value Uniform value
     * \return Return true if the uniform has been set successfully
     */
    bool setUniform(const std::string& name, const Values& value);

    /**
     * Set the given uniform
     * \param name Uniform name
     * \param mat Uniform value: a 4x4 matrix
     * \return Return true if the uniform has been set successfully
     */
    bool setUniform(const std::string& name, const glm::mat4 mat);

    /**
     * Set the varyings for transform feedback
     * \param varyingNames Names of the outputs
     */
    void selectVaryings(const std::vector<std::string>& varyingNames);

  private:
    GLuint _program;
    ProgramType _type;
    std::string _programName{""};

    std::map<gfx::ShaderType, const ShaderStage*> _shaderStages;
    std::map<std::string, Uniform> _uniforms;
    std::map<std::string, std::string> _uniformsDocumentation;
    std::vector<std::string> _uniformsToUpdate;

    bool _isLinked{false};
    bool _isActive{false};

    /**
     * Parse the uniforms in the given shader source
     * \param source Shader source
     */
    void parseUniforms(std::string_view source);

    /**
     * Set the currently queued uniforms updates
     */
    void updateUniforms();
};

} // namespace Splash::gfx::opengl

#endif
