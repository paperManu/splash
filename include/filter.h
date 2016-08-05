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

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "image.h"
#include "object.h"
#include "texture.h"
#include "texture_image.h"

namespace Splash {

/*************/
class Filter : public Texture
{
    public:
        /**
         * Constructor
         */
        Filter(std::weak_ptr<RootObject> root);

        /**
         * Destructor
         */
        ~Filter();

        /**
         * No copy constructor, but a move one
         */
        Filter(const Filter&) = delete;
        Filter(Filter&&) = default;
        Filter& operator=(const Filter&) = delete;

        /**
         * Bind / unbind this texture of this filter
         */
        void bind();
        void unbind();

        /**
         * Get the shader parameters related to this texture
         * Texture should be locked first
         */
        std::unordered_map<std::string, Values> getShaderUniforms() const;

        /**
         * Get spec of the texture
         */
        ImageBufferSpec getSpec() const {return _outTextureSpec;}

        /**
         * Try to link / unlink the given BaseObject to this
         */
        bool linkTo(std::shared_ptr<BaseObject> obj);
        void unlinkFrom(std::shared_ptr<BaseObject> obj);

        /**
         * Filters should always be saved as it holds user-modifiable parameters
         */
        void setSavable(bool savable) {_savable = true;}

        /**
         * Update the texture according to the owned Image
         */
        void update();

    private:
        bool _isInitialized {false};
        std::shared_ptr<GlWindow> _window;
        std::weak_ptr<Texture> _inTexture;

        GLuint _fbo {0};
        std::shared_ptr<Texture_Image> _outTexture {nullptr};
        std::shared_ptr<Object> _screen;
        ImageBufferSpec _outTextureSpec;

        // Filter parameters
        bool _render16bits {false};
        bool _updateColorDepth {false}; // Set to true if the _render16bits has been updated
        float _blackLevel {0.f};
        float _brightness {1.f};
        float _colorTemperature {6500.f};
        float _contrast {1.f};
        float _saturation {1.f};

        // Computed values
        glm::vec2 _colorBalance {1.f, 1.f};

        /**
         * Init function called in constructors
         */
        void init();

        /**
         * Setup the output texture
         */
        void setOutput();

        /**
         * Update the color depth for all textures
         */
        void updateColorDepth();

        /**
         * Updates the shader uniforms according to the textures and images
         * the filter is connected to.
         */
        void updateUniforms();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_FILTER_H
