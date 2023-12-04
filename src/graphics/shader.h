/*
 * Copyright (C) 2013 Splash authors
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
 * @shader.h
 * The Shader class
 */

#ifndef SPLASH_SHADER_H
#define SPLASH_SHADER_H

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/graph_object.h"
#include "./graphics/api/opengl/shader_gfx_impl.h"
#include "./graphics/texture.h"

namespace Splash
{

class Shader final : public GraphObject
{
  public:
    /*
     * Enum for shader program types
     */
    enum ProgramType
    {
        prgGraphic = 0,
        prgCompute,
        prgFeedback
    };

    enum Sideness
    {
        doubleSided = 0,
        singleSided,
        inverted
    };

    /**
     * Enum for shader compute phases
     */
    enum class ComputePhase : uint8_t
    {
        ResetVisibility,
        ResetBlending,
        ComputeCameraContribution,
        TransferVisibilityToAttribute
    };

    /**
     * Enum for shader feedback phases
     */
    enum class FeedbackPhase : uint8_t
    {
        TessellateFromCamera
    };

  public:
    /**
     * Constructor
     * \param type Shader type
     */
    explicit Shader(RootObject* root, ProgramType type = prgGraphic);

    /**
     * Destructor
     */
    ~Shader() override final = default;

    /**
     * Constructors/operators
     */
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    /**
     * Activate this shader
     */
    void activate();

    /**
     * Deactivate this shader
     */
    void deactivate();

    /**
     * Launch the compute shader, if present
     * \param numGroupsX Compute group count along X
     * \param numGroupsY Compute group count along Y
     * \return Return true if the computation went well
     */
    bool doCompute(GLuint numGroupsX = 1, GLuint numGroupsY = 1);

    /**
     * Set the sideness of the object
     * \param side Sideness
     */
    void setSideness(const Sideness side);

    /**
     * Get the sideness of the object
     * \return Return the sideness
     */
    Sideness getSideness() const { return _sideness; }

    /**
     * Get the list of uniforms in the shader program
     * \return Return a map of uniforms and their values
     */
    std::map<std::string, Values> getUniforms() const;

    /**
     * Get the documentation for the uniforms based on the comments in GLSL code
     * \return Return a map of uniforms and their documentation
     */
    std::map<std::string, std::string> getUniformsDocumentation() const;

    /**
     * Select the compute phase to activate, with some arguments
     * \param phase Compute phase to select
     */
    void selectComputePhase(ComputePhase phase);

    /**
     * Select the feedback phase to activate
     * \param phase Feedback phase to select
     * \param varyings Selected feedback varyings
     */
    void selectFeedbackPhase(FeedbackPhase phase, const std::vector<std::string>& varyings = {});

    /**
     * Select the fill mode for graphic shaders
     * \param mode Fill mode
     * \param parameters Fill parameters
     */
    void selectFillMode(std::string_view mode, const std::vector<std::string>& parameters = {});

    /**
     * Set the model view and projection matrices
     * \param mv View matrix
     * \param mp Projection matrix
     */
    void setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp);

    /**
     * Set a shader source
     * \param type Shader type
     * \param src Shader string
     * \return Return true if the shader was compiled successfully
     */
    bool setSource(const gfx::ShaderType type, const std::string& src);

    /**
     * Set multiple shaders at once
     * \param sources Map of shader sources
     * \return Return true if all shader could be compiled
     */
    bool setSource(const std::map<gfx::ShaderType, std::string>& sources);

    /**
     * Set a shader source from file
     * \param type Shader type
     * \param filename Shader file
     * \return Return true if the shader was compiled successfully
     */
    bool setSourceFromFile(const gfx::ShaderType type, const std::string& filename);

    /**
     * Add a new texture to use
     * \param texture Texture
     * \param textureUnit GL texture unit
     * \param name Texture name
     */
    void setTexture(const std::shared_ptr<Texture>& texture, const GLuint textureUnit, const std::string& name);

    /**
     * Set the named uniform to the given value, if it exists.
     * It can be called anytime and will be updated once the shader is active
     * \param name Uniform name
     * \param value Uniform value
     */
    void setUniform(const std::string& name, const Value& value);

  private:
    ProgramType _programType{prgGraphic};
    std::unique_ptr<gfx::ShaderGfxImpl> _gfxImpl;

    std::unordered_map<int, std::string> _currentSources;
    std::vector<std::shared_ptr<Texture>> _textures; // Currently used textures
    std::string _currentProgramName{};

    // Rendering parameters
    std::string _shaderOptions{""};
    Sideness _sideness{doubleSided};

    /**
     * Parses the shader to replace includes by the corresponding sources
     * \param src Shader source
     * \return Return the source with includes parsed
     */
    std::string parseIncludes(const std::string& src);

    /**
     * Register new functors to modify attributes
     */
    void registerGraphicAttributes();
};

} // namespace Splash

#endif // SPLASH_SHADER_H
