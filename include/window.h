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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @window.h
 * The Window class
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <config.h>

#include <memory>
#include <vector>

#include "coretypes.h"
#include "geometry.h"
#include "log.h"
#include "object.h"
#include "texture.h"

namespace Splash {

class Window : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Window(GlWindowPtr w);

        /**
         * Destructor
         */
        ~Window();

        /**
         * No copy constructor, but a move one
         */
        Window(const Window&) = delete;
        Window(Window&& w)
        {
            _isInitialized = w._isInitialized;
            _window = w._window;
            _screen = w._screen;
            _inTextures = w._inTextures;
        }

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Render this window to screen
         */
        void render();

        /**
         * Set a new texture to draw
         */
        void setTexture(TexturePtr tex);

    private:
        bool _isInitialized {false};
        GlWindowPtr _window;

        ObjectPtr _screen;
        std::vector<TexturePtr> _inTextures;
};

typedef std::shared_ptr<Window> WindowPtr;

} // end of namespace

#endif // WINDOW_H
