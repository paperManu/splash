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
 * @image.h
 * The Image class
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <chrono>
#include <OpenImageIO/imagebuf.h>

#include "config.h"
#include "coretypes.h"
#include "log.h"

using namespace OIIO_NAMESPACE;

namespace Splash {

class Image : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Image();

        /**
         * Destructor
         */
        virtual ~Image();

        /**
         * Copy and move constructors
         */
        Image(const Image& i)
        {
            _image.copy(i._image);
        }
        
        Image(Image&& i)
        {
            _image.swap(i._image);
        }

        /**
         * = operator
         */
        Image& operator=(const Image& i)
        {
            _image.copy(i._image);
            return *this;
        }

        /**
         * Get the image buffer
         */
        ImageBuf get() const;

        /**
         * Get the timestamp for the current mesh
         */
        std::chrono::high_resolution_clock::time_point getTimestamp() const {return _timestamp;}

        /**
         * Set the image from an ImageBuf
         */
        void set(const ImageBuf& img);

        /**
         * Serialize the image
         */
        SerializedObject serialize() const;

        /**
         * Update the Image from a serialized representation
         */
        bool deserialize(const SerializedObject& obj);

        /**
         * Update the content of the image
         */
        virtual void update() {}

    protected:
        ImageBuf _image;
        std::chrono::high_resolution_clock::time_point _timestamp;

    private:
        /**
         * Create a default pattern
         */
        void createDefaultImage();

        void updateTimestamp();
};

typedef std::shared_ptr<Image> ImagePtr;

} // end of namespace

#endif // IMAGE_H
