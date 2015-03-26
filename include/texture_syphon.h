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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
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
#include "texture_syphon_client.h"

namespace Splash
{

/**************/
class Texture_Syphon : public BaseObject
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
         * Bind / unbind this texture
         */
        void bind();
        void unbind();

    private:
        SyphonReceiver _syphonReceiver;
        std::string _serverName {""};
        std::string _appName {""};

        GLint _activeTexture;

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Texture_Syphon> Texture_SyphonPtr;

} // end of namespace

#endif
