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
 * Base class for Pixel Buffer Objects API implementations
 */

#ifndef SPLASH_PBO_GFX_IMPL_H
#define SPLASH_PBO_GFX_IMPL_H

#include <cstdint>

namespace Splash
{

class Texture;

namespace gfx
{

class PboGfxImpl
{
  public:
    /**
     * Constructor
     * \param size Buffer count to create
     */
    PboGfxImpl(std::size_t size)
        : _pboCount(size)
    {
    }

    /**
     * Destructor
     */
    virtual ~PboGfxImpl() = default;

    /**
     * Get the number of underlying pixel buffer objects
     * \return Return the PBO count
     */
    inline std::size_t getPBOCount() const { return _pboCount; }

    /**
     * Activate the given PBO index for pixel packing (in other words, as sink)
     * \param index PBO index to activate
     */
    virtual void activatePixelPack(std::size_t index) = 0;

    /**
     * Deactivate any PBO from pixel packing
     */
    virtual void deactivatePixelPack() = 0;

    /**
     * Pack the given texture to the underlying PBO active for pixel packing
     * This must be called after activatePixelPack
     * \param texture Texture to pack
     */
    virtual void packTexture(Texture* texture) = 0;

    /**
     * Map the given PBO index to be read from the CPU
     * \param index PBO index to map
     * \return Return a pointer to the first byte of the mapped memory
     */
    virtual uint8_t* mapRead(std::size_t index) = 0;

    /**
     * Unmap any PBO from being read by the CPU
     */
    virtual void unmapRead() = 0;

    /**
     * Update the underlying PBOs to match the given size
     * If the size is not changed, the PBOs are not recreated
     * \param size New size for the PBOs
     */
    virtual void updatePBOs(std::size_t size) = 0;

  protected:
    const std::size_t _pboCount;
};

} // namespace gfx
} // namespace Splash

#endif
