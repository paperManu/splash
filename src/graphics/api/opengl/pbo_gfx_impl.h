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
 * @pbo_gfx_impl.h
 * Implementation of PboGfxImpl for OpenGL
 */

#ifndef SPLASH_OPENGL_PBO_GFX_IMPL_H
#define SPLASH_OPENGL_PBO_GFX_IMPL_H

#include <vector>

#include "./core/constants.h"
#include "./graphics/api/pbo_gfx_impl.h"

namespace Splash::gfx::opengl
{

class PboGfxImpl : public gfx::PboGfxImpl
{
  public:
    /**
     * Constructor
     * \param size Buffer count to create
     */
    PboGfxImpl(std::size_t size);

    /**
     * Destructor
     */
    virtual ~PboGfxImpl();

    /**
     * Activate the given PBO index for pixel packing (in other words, as sink)
     * \param index PBO index to activate
     */
    void activatePixelPack(std::size_t index) override final;

    /**
     * Deactivate any PBO from pixel packing
     */
    void deactivatePixelPack() override final;

    /**
     * Pack the given texture to the underlying PBO active for pixel packing
     * This must be called after activatePixelPack
     * \param texture Texture to pack
     * \param bpp Pixel size in bits
     */
    void packTexture(Texture* texture) override final;

    /**
     * Map the given PBO index to be read from the CPU
     * \param index PBO index to map
     * \return Return a pointer to the first byte of the mapped memory
     */
    uint8_t* mapRead(std::size_t index) override final;

    /**
     * Unmap any PBO from being read by the CPU
     */
    void unmapRead() override final;

    /**
     * Update the underlying PBOs to match the given size
     * If the size is not changed, the PBOs are not recreated
     * \param size New size for the PBOs
     */
    void updatePBOs(std::size_t size) override final;

  private:
    std::vector<GLuint> _pbos{};
    std::size_t _bufferSize{0};

    GLint _pboMapReadIndex{0};
    uint8_t* _mappedPixels{nullptr};
};

} // namespace Splash::gfx::opengl

#endif
