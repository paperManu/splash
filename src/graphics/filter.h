/*
 * Copyright (C) 2015 Emmanuel Durand
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
 * @filter.h
 * The Filter class
 */

#ifndef SPLASH_FILTER_H
#define SPLASH_FILTER_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./graphics/framebuffer.h"
#include "./image.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"

namespace Splash
{

/*************/
class Filter : public Texture
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Filter(RootObject* root);

    /**
     * \brief Destructor
     */
    ~Filter() override;

    /**
     * No copy constructor, but a move one
     */
    Filter(const Filter&) = delete;
    Filter(Filter&&) = default;
    Filter& operator=(const Filter&) = delete;

    /**
     * \brief Bind the filter
     */
    void bind();

    /**
     * \brief Unbind the filter
     */
    void unbind();

    /**
     * Get the output texture
     * \return Return the output texture
     */
    std::shared_ptr<Texture_Image> getOutTexture() const { return _fbo->getColorTexture(); }

    /**
     * Get the shader parameters related to this texture
     * Texture should be locked first
     */
    std::unordered_map<std::string, Values> getShaderUniforms() const;

    /**
     * \brief Get specs of the texture
     * \return Return the texture specs
     */
    ImageBufferSpec getSpec() const { return _fbo->getColorTexture()->getSpec(); }

    /**
     * \brief Get the id of the gl texture
     * \return Return the texture id
     */
    GLuint getTexId() const { return _fbo->getColorTexture()->getTexId(); }

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
     * Set whether to keep the input image ratio
     * \param keepRatio Keep ratio if true
     */
    void setKeepRatio(bool keepRatio);

    /**
     * \brief Filters should always be saved as it holds user-modifiable parameters
     * \param savable Needed for heritage reasons, no effect whatsoever
     */
    void setSavable(bool /*savable*/) override { _savable = true; }

    /**
     * \brief Render the filter
     */
    void render() override;

    /**
     * \brief Update for a filter does nothing, it is the render() job
     */
    void update() override {}

  private:
    std::vector<std::weak_ptr<Texture>> _inTextures;

    std::unique_ptr<Framebuffer> _fbo{nullptr};
    std::shared_ptr<Object> _screen;
    ImageBufferSpec _outTextureSpec;

    // Filter parameters
    int _sizeOverride[2]{-1, -1};                            //!< If set to positive values, overrides the size given by input textures
    bool _keepRatio{false};
    std::unordered_map<std::string, Values> _filterUniforms; //!< Contains all filter uniforms
    bool _render16bits{false};                               //!< Set to true for the filter to be rendered in 16bits
    Values _colorCurves{};                                   //!< RGB points for the color curves, active if at least 3 points are set
    float _autoBlackLevelTargetValue{0.f};                   //!< If not zero, defines the target luminance value
    float _autoBlackLevelSpeed{0.02f};                       //!< Coefficient applied to update the black level value
    float _autoBlackLevel{0.f};

    std::string _shaderSource{""};     //!< User defined fragment shader filter
    std::string _shaderSourceFile{""}; //!< User defined fragment shader filter source file

    /**
     * \brief Init function called in constructors
     */
    void init();

    /**
     * \brief Set the filter fragment shader. Automatically adds attributes corresponding to the uniforms
     * \param source Source fragment shader
     * \return Return true if the shader is valid
     */
    bool setFilterSource(const std::string& source);

    /**
     * \brief Setup the output texture
     */
    void setOutput();

    /**
     * \brief Updates the shader uniforms according to the textures and images the filter is connected to.
     */
    void updateUniforms();

    /**
     * Update the shader parameters, if it is the default shader
     */
    void updateShaderParameters();

    /**
     * Update the size override to take (or not) ratio into account
     */
    void updateSizeWrtRatio();

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();

    /**
     * Register attributes related to the default shader
     */
    void registerDefaultShaderAttributes();
};

} // end of namespace

#endif // SPLASH_FILTER_H
