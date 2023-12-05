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
 * @compute_shader.h
 * Class for compute shader, implementated for OpenGL ES
 */

#ifndef SPLASH_GLES_COMPUTE_SHADER_GFX_IMPL_H
#define SPLASH_GLES_COMPUTE_SHADER_GFX_IMPL_H

#include "./core/constants.h"
#include "./graphics/api/compute_shader.h"
#include "./graphics/api/gles/shader_gfx_impl.h"

namespace Splash::gfx::gles
{

class ComputeShaderGfxImpl : public gfx::ComputeShaderGfxImpl, public ShaderGfxImpl
{
  public:
    /**
     * Constructor
     */
    ComputeShaderGfxImpl();

    /**
     * Destructor
     */
    ~ComputeShaderGfxImpl() override = default;

    /**
     * Launch the compute shader
     * \param numGroupsX Compute group count along X
     * \param numGroupsY Compute group count along Y
     * \return Return true if computation succeeded
     */
    bool compute(uint32_t numGroupsX = 1, uint32_t numGroupsY = 1) override final;
};

} // namespace Splash::gfx::gles

#endif
