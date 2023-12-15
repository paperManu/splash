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
 * Base class for graphic shader, implementation specific rendering API
 */

#ifndef SPLASH_GRAPHIC_SHADER_GFX_IMPL_H
#define SPLASH_GRAPHIC_SHADER_GFX_IMPL_H

#include <string_view>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

#include "./core/constants.h"
#include "./graphics/api/shader_gfx_impl.h"
#include "./graphics/texture.h"

namespace Splash::gfx
{

class GraphicShaderGfxImpl : virtual public ShaderGfxImpl
{
  public:
    /**
     * Constructor
     */
    GraphicShaderGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~GraphicShaderGfxImpl() override = default;

    /**
     * Activate the shader
     * \return Return true if the activation succeeded
     */
    virtual bool activate() override = 0;

    /**
     * Deactivate the program
     */
    virtual void deactivate() override = 0;

    /**
     * Set the model view and projection matrices
     * \param mv View matrix
     * \param mp Projection matrix
     */
    virtual void setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp) = 0;

    /**
     * Specify a texture to use with the shader.
     * Must be called after activating the shader
     * \param texture Texture to be used
     * \param textureUnit Texture unit that should be used
     * \param name Uniform name, in the shader source, for this texture
     * \return Return true if it succeeded
     */
    virtual bool setTexture(Texture* texture, const GLuint textureUnit, std::string_view name) = 0;

    /**
     * Unset all the textures.
     * Must be called before deactivating the shader
     */
    virtual void unsetTextures() = 0;

  protected:
    std::vector<Texture*> _textures;
};

} // namespace Splash::gfx

#endif
