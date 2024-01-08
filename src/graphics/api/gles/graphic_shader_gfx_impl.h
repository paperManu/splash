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
 * @graphic_shader_gfx_impl.h
 * Class for graphic shader, implementated for OpenGL ES
 */

#ifndef SPLASH_GLES_GRAPHIC_SHADER_GFX_IMPL_H
#define SPLASH_GLES_GRAPHIC_SHADER_GFX_IMPL_H

#include <string_view>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

#include "./core/constants.h"
#include "./graphics/api/gles/shader_gfx_impl.h"
#include "./graphics/api/gles/shader_program.h"
#include "./graphics/api/gles/shader_stage.h"
#include "./graphics/api/graphic_shader_gfx_impl.h"
#include "./graphics/texture.h"

namespace Splash::gfx::gles
{

class GraphicShaderGfxImpl : public gfx::GraphicShaderGfxImpl, public ShaderGfxImpl
{
  public:
    /**
     * Constructor
     */
    GraphicShaderGfxImpl();

    /**
     * Destructor
     */
    virtual ~GraphicShaderGfxImpl() override = default;

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
     * Set the model view and projection matrices
     * \param mv View matrix
     * \param mp Projection matrix
     */
    void setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp) override final;

    /**
     * Specify a texture to use with the shader.
     * Must be called after activating the shader
     * \param texture Texture to be used
     * \param textureUnit Texture unit that should be used
     * \param name Uniform name, in the shader source, for this texture
     * \return Return true if it succeeded
     */
    bool setTexture(Texture* texture, const GLuint textureUnit, std::string_view name) override final;

    /**
     * Unset all the textures.
     * Must be called before deactivating the shader
     */
    void unsetTextures() override final;
};

} // namespace Splash::gfx::gles

#endif
