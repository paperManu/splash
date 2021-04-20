/*
 * Copyright (C) 2020 Marie-Eve Dumas
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
 * @image_sequence.h
 * The Image_Sequence class
 */

#ifndef SPLASH_IMAGE_SEQUENCE_H
#define SPLASH_IMAGE_SEQUENCE_H

#include "./image/image.h"

namespace Splash
{

class Image_Sequence : public Image
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    Image_Sequence(RootObject* root)
        : Image(root)
    {
        init();
    };

    /**
     * No copy constructors
     */
    Image_Sequence(const Image_Sequence&) = delete;
    Image_Sequence& operator=(const Image_Sequence&) = delete;

    /**
     * Set the path to read from.
     * \param filename
     */
    virtual bool read(const std::string& dirname) = 0;

    /**
     * Obtain a new image
     */
    virtual bool capture() = 0;

  protected:
    /**
     * Base init for the class
     */
    void init()
    {
        _type = "image_sequence";
        registerAttributes();
    }

    /**
     * Register new functors to modify attributes
     */
    virtual void registerAttributes()
    {
        Image::registerAttributes();

        addAttribute(
            "capture",
            [&](const Values&) {
                runAsyncTask([=]() { capture(); });
                return true;
            },
            // Allows for showing a capture button in the GUI
            [&]() -> Values { return {false}; },
            {});
        setAttributeDescription("capture", "Ask for the camera to obtain an image");
    }
};

} // namespace Splash

#endif // SPLASH_IMAGE_SEQUENCE_H
