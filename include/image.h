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

#ifndef SPLASH_IMAGE_H
#define SPLASH_IMAGE_H

#include <chrono>
#include <mutex>
#include <OpenImageIO/imagebuf.h>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"

namespace oiio = OIIO_NAMESPACE;

namespace Splash {

class Image : public BufferObject
{
    public:
        /**
         * Constructor
         */
        Image();
        Image(bool linked);
        Image(oiio::ImageSpec spec);

        /**
         * Destructor
         */
        virtual ~Image();

        /**
         * No copy, but some move constructors
         */
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        /**
         * Get a pointer to the data
         */
        const void* data() const;

        /**
         * Lock the image, useful while reading. Use with care
         * Note that only write mutex is needed, as it also disables reading
         */
        void lock() {_writeMutex.lock();}
        void unlock() {_writeMutex.unlock();}

        /**
         * Get the image buffer
         */
        oiio::ImageBuf get() const;

        /**
         * Get the image buffer specs
         */
        oiio::ImageSpec getSpec() const;

        /**
         * Get the timestamp for the current mesh
         */
        std::chrono::high_resolution_clock::time_point getTimestamp() const {return _timestamp;}

        /**
         * Set the image from an ImageBuf
         */
        void set(const oiio::ImageBuf& img);

        /**
         * Set the image as a empty with the given size / channels / typedesc
         */
        void set(unsigned int w, unsigned int h, unsigned int channels, oiio::TypeDesc type);

        /**
         * Serialize the image
         */
        std::unique_ptr<SerializedObject> serialize() const;

        /**
         * Update the Image from a serialized representation
         */
        bool deserialize(std::unique_ptr<SerializedObject> obj);

        /**
         * Set the path to read from
         */
        virtual bool read(const std::string& filename);

        /**
         * Set all pixels in the image to the specified value
         */
        void setTo(float value);

        /**
         * Update the content of the image
         */
        virtual void update();

        /**
         * Write the current buffer to the specified file
         */
        bool write(const std::string& filename);

    protected:
        oiio::ImageBuf _image;
        oiio::ImageBuf _bufferImage;
        bool _flip {false};
        bool _flop {false};
        bool _imageUpdated {false};
        bool _srgb {true};
        bool _benchmark {false};
        bool _linkedToWorldObject {false};

        void createDefaultImage(); //< Create a default pattern

        /**
         * Read the specified image file
         */
        bool readFile(const std::string& filename);
        
    private:
        // Deserialization is done in this buffer, to avoid realloc
        oiio::ImageBuf _bufferDeserialize;

        /**
         * Base init for the class
         */
        void init();
        
        /**
         * Register new functors to modify attributes
         */
        virtual void registerAttributes();
};

typedef std::shared_ptr<Image> ImagePtr;

} // end of namespace

#endif // SPLASH_IMAGE_H
