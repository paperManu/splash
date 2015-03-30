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

#ifndef SPLASH_TEXTURESYPHONCLIENT_H
#define SPLASH_TEXTURESYPHONCLIENT_H

#include <string>

namespace Splash
{

class SyphonReceiver
{
    public:
        SyphonReceiver();
        ~SyphonReceiver();

        bool connect(const char* serverName = "", const char* appName = "");
        void disconnect();
        bool isConnected() {return _syphonClient != nullptr;}

        int getFrame();
        int getHeight() {return _height;}
        int getWidth() {return _width;}
        void releaseFrame();

    private:
        void* _syphonClient;
        void* _syphonImage;
        void* _sharedDirectory;
        int _width, _height;
};

} // end of namespace

#endif
