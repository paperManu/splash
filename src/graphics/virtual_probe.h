/*
 * Copyright (C) 2013 Emmanuel Durand
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
 * @virtual_probe.h
 * The VirtualProbe class
 */

#ifndef SPLASH_VIRTUAL_SCREEN_H
#define SPLASH_VIRTUAL_SCREEN_H

#include <math.h>

#include <glm/glm.hpp>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./graphics/framebuffer.h"
#include "./graphics/geometry.h"
#include "./image/image.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"

namespace Splash
{

/*************/
class VirtualProbe : public Texture
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    VirtualProbe(RootObject* root);

    /**
     * Destructor
     */
    ~VirtualProbe() override;

    /**
     * No copy constructor or operator
     */
    VirtualProbe(const VirtualProbe&) = delete;
    VirtualProbe(VirtualProbe&&) = default;
    VirtualProbe& operator=(const VirtualProbe&) = delete;

    /**
     * \brief Compute the matrix corresponding to the virtual screen position
     * \return Return the view matrix
     */
    glm::dmat4 computeViewMatrix() const;

    /**
     * \brief Get the shader parameters related to this warp. Texture should be locked first.
     * \return Return the shader uniforms
     */
    std::unordered_map<std::string, Values> getShaderUniforms() const;

    /**
     * \brief Get spec of the texture
     * \return Return the spec
     */
    ImageBufferSpec getSpec() const { return _outFbo->getColorTexture()->getSpec(); }

    /**
     * \brief Try to link the given BaseObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(const std::shared_ptr<BaseObject>& obj) override;

    /**
     * \brief Try to unlink the given BaseObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(const std::shared_ptr<BaseObject>& obj) override;

    /**
     * \brief Render this camera into its textures
     */
    void render() override;

    /**
     * \brief Set the output framebuffer
     */
    void setupFBO();

    /**
     * \brief Set the resolution of this VirtualProbe
     * \param width Width of the output textures
     * \param height Height of the output textures
     */
    void setOutputSize(int width, int height);

    /**
     * \brier Bind this warp
     */
    void bind() final;

    /**
     * \brier Unbind this warp
     */
    void unbind() final;

    /**
     * \brief Get the id of the gl texture
     * \return Return the texture id
     */
    GLuint getTexId() const { return _outFbo->getColorTexture()->getTexId(); }

    /**
     * \brief Update for a VirtualProbe does nothing, it is the render() job
     */
    void update() final {}

  private:
    enum ProjectionType
    {
        Equirectangular = 0,
        Spherical = 1
    };

  private:
    std::unique_ptr<Framebuffer> _fbo{nullptr};
    std::unique_ptr<Framebuffer> _outFbo{nullptr};
    std::vector<std::weak_ptr<Object>> _objects{};
    std::unique_ptr<Object> _screen{nullptr};

    glm::dmat4 _faceProjectionMatrix{1.0};
    glm::dvec3 _position{0.0, 0.0, 0.0};
    glm::dvec3 _rotation{0.0, 90.0, 0.0};
    uint32_t _width{2048}, _height{2048};
    uint32_t _newWidth{0}, _newHeight{0};
    uint32_t _cubemapSize{512};

    ProjectionType _projectionType{ProjectionType::Equirectangular};
    float _sphericalFov{180.f};

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif
