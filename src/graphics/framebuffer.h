/*
 * Copyright (C) 2017 Emmanuel Durand
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
 * @fbo.h
 * The framebuffer object class
 */

#ifndef SPLASH_FBO_H
#define SPLASH_FBO_H

#include <memory>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/graph_object.h"
#include "./graphics/texture_image.h"

namespace Splash
{

/*************/
class Framebuffer : public GraphObject
{
  public:
    /**
     * Constructor
     */
    Framebuffer(RootObject* root);

    /**
     * Destructor
     */
    ~Framebuffer() override;

    /**
     * No copy constructor
     */
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    /**
     * Bind the FBO to draw into it
     */
    void bindDraw();
    void bindRead();

    /**
     * Blit a FBO into another
     * \param src Source FBO
     * \param dst Destination FBO
     */
    static void blit(const Framebuffer& src, const Framebuffer& dst);

    /**
     * Get the color texture
     * \return The color texture
     */
    std::shared_ptr<Texture_Image> getColorTexture() const { return _colorTexture; }

    /**
     * Get the depth texture
     * \return The depth texture
     */
    std::shared_ptr<Texture_Image> getDepthTexture() const { return _depthTexture; }

    /**
     * Get the depth at the given location
     * \param x X position
     * \param y Y position
     * \return Return the depth
     */
    float getDepthAt(float x, float y);

    /**
     * Get the GL FBO id
     * \return The FBO id
     */
    GLuint getFboId() const { return _fbo; }

    /**
     * Get the size of the FBO
     */
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }

    /**
     * Set the FBO multisampling and bit per channel settings
     * \param multisample Multisampling value
     * \param sixteenbpc If true, set color depth to 16bpc
     * \param srgb If true, use sRGB color space (overrides 16bpc)
     * \param cubemap True if render to cubemap
     */
    void setParameters(int multisample, bool sixteenbpc, bool srgb = false, bool cubemap = false);

    /**
     * Set whether this FBO should resize automatically
     * \param autoresize Set to true for auto resizing
     */
    void setAutoResize(bool autoresize) { _automaticResize = autoresize; }

    /**
     * Set the FBO size
     * \param width Width
     * \param height Height
     */
    void setSize(int width, int height);

    /**
     * Set the FBO to be resizable automatically
     * \param resizable If true, automatically resizable
     */
    void setResizable(bool resizable);

    /**
     * Unbind the FBO
     */
    void unbindDraw();
    void unbindRead();

  private:
    GLuint _fbo{0};
    std::shared_ptr<Texture_Image> _depthTexture{nullptr};
    std::shared_ptr<Texture_Image> _colorTexture{nullptr};

    int _width{512}, _height{512};
    int _multisample{0};
    bool _16bits{false};
    bool _srgb{false};
    bool _automaticResize{false}; // TODO: handle this correctly
    int _previousFbo{0};
};

} // namespace Splash

#endif // SPLASH_FBO_H
