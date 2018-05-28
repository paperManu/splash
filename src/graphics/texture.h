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
 * @texture.h
 * The Texture base class
 */

#ifndef SPLASH_TEXTURE_H
#define SPLASH_TEXTURE_H

#include <chrono>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/graph_object.h"
#include "./core/imagebuffer.h"

namespace Splash
{

class Texture : public GraphObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Texture(RootObject* root);

    /**
     * \brief Destructor
     */
    virtual ~Texture();

    /**
     * No copy constructor, but a move one
     */
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    /**
     * \brief Bind this texture
     */
    virtual void bind() = 0;

    /**
     * \brief Unbind this texture
     */
    virtual void unbind() = 0;

    /**
     * \brief Get the shader parameters related to this texture. Texture should be locked first.
     * \return Return the shader uniforms
     */
    virtual std::unordered_map<std::string, Values> getShaderUniforms() const = 0;

    /**
     * \brief Get spec of the texture
     * \return Return the texture spec
     */
    virtual ImageBufferSpec getSpec() const = 0;

    /**
     * Get the output texture GL id
     * \return Return the id
     */
    virtual GLuint getTexId() const = 0;

    /**
     * \brief Get the prefix for the glsl sampler name
     */
    virtual std::string getPrefix() const { return "_tex"; }

    /**
     * \brief Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    virtual bool linkTo(const std::shared_ptr<GraphObject>& obj);

    /**
     * \brief Lock the texture for read / write operations
     */
    void lock() const { _mutex.lock(); }

    /**
     * \brief Unlock the texture for read / write operations
     */
    void unlock() const { _mutex.unlock(); }

    /**
     * \brief Set whether the texture should be resizable
     * \param resizable If true, the texture is resizable
     */
    void setResizable(bool resizable) { _resizable = resizable; }

    /**
     * \brief Update the texture according to the owned Image
     */
    virtual void update() = 0;

  protected:
    mutable std::mutex _mutex;
    ImageBufferSpec _spec;

    // Store some texture parameters
    bool _resizable{true};

    int64_t _timestamp;

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();

  private:
    /**
     * \brief As says its name
     */
    void init();
};

} // namespace Splash

#endif // SPLASH_TEXTURE_H
