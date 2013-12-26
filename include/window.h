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

#include "object.h"
#include "texture.h"

namespace Splash {

class Window {
    public:
        /**
         * Constructor
         */
        Window();

        /**
         * Destructor
         */
        ~Window();

    private:
        ObjectPtr _screen;
        std::vector<TexturePtr> _inTextures;
};

typedef std::shared_ptr<Window> WindowPtr;

} // end of namespace

#endif // WINDOW_H
