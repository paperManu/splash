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

/**
 * @gui_gfx_impl.h
 * Base class for graphics calls used in the Gui class
 */

#ifndef SPLASH_GUI_GFX_IMPL_H
#define SPLASH_GUI_GFX_IMPL_H

#include <imgui.h>

#include "./core/constants.h"

namespace Splash::gfx
{

class GuiGfxImpl
{
  public:
    /**
     * Constructor
     */
    GuiGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~GuiGfxImpl() = default;

    /**
     * Initialize the shaders and vertex buffer
     */
    virtual void initRendering() = 0;

    /**
     * Initialize fonts texture
     * \param width Font texture width
     * \param height Font texture height
     * \param pixels Font texture data
     * \return Return the ID for the texture font
     */
    virtual ImTextureID initFontTexture(uint32_t width, uint32_t height, uint8_t* pixels) = 0;

    /**
     * Setup the rendering viewport
     * \param width Viewport width
     * \param height Viewport height
     */
    virtual void setupViewport(uint32_t width, uint32_t height) = 0;

    /**
     * Render the GUI
     * \param width Display width
     * \param height Display height
     * \param drawData Data to draw, as generated by ImGui
     */
    virtual void drawGui(float width, float height, ImDrawData* drawData) = 0;
};

} // namespace Splash::gfx

#endif