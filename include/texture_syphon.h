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
 * @texture_texture.h
 * The Texture_Syphon base class
 */

#ifndef SPLASH_TEXTURESYPHON_H
#define SPLASH_TEXTURESYPHON_H

#include <string>
#include <vector>

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"
#include "texture.h"
#include "texture_syphon_client.h"

namespace Splash
{

/**************/
class Texture_Syphon : public Texture
{
    public:
        /**
         * Constructor
         */
        Texture_Syphon();

        /**
         * Destructor
         */
        ~Texture_Syphon();

        /**
         * No copy constructor, but a move one
         */
        Texture_Syphon(const Texture_Syphon&) = delete;
        Texture_Syphon& operator=(const Texture_Syphon&) = delete;

        /**
         * Bind / unbind this texture
         */
        void bind();
        void unbind();

        /**
         * Get the shader parameters related to this texture
         * Texture should be locked first
         */
        std::unordered_map<std::string, Values> getShaderUniforms() const {return _shaderUniforms;}

        /**
         * Get spec of the texture
         */
        oiio::ImageSpec getSpec() const {return oiio::ImageSpec();}

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(std::shared_ptr<BaseObject> obj);

        /**
         * Update the texture according to the owned Image
         */
        void update() {};

    private:
        SyphonReceiver _syphonReceiver;
        std::string _serverName {""};
        std::string _appName {""};

        GLint _activeTexture;

        // Parameters to send to the shader
        std::unordered_map<std::string, Values> _shaderUniforms;

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Texture_Syphon> Texture_SyphonPtr;

} // end of namespace

#endif
