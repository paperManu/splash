/*
 * Copyright (C) 2014 Emmanuel Durand
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
 * @image_shmdata.h
 * The Image_Shmdata class
 */

#ifndef SPLASH_IMAGE_SHMDATA_H
#define SPLASH_IMAGE_SHMDATA_H

#include <shmdata/any-data-reader.h>
#include <shmdata/any-data-writer.h>

#include "config.h"
#include "image.h"

namespace oiio = OIIO_NAMESPACE;

namespace Splash
{

class Image_Shmdata : public Image
{
    public:
        /**
         * Constructor
         */
        Image_Shmdata();

        /**
         * Destructor
         */
        ~Image_Shmdata();

        /**
         * No copy constructor, only move
         */
        Image_Shmdata(const Image_Shmdata&) = delete;
        Image_Shmdata& operator=(const Image_Shmdata&) = delete;

        Image_Shmdata(Image_Shmdata&& g) noexcept
        {
            *this = std::move(g);
        }

        Image_Shmdata& operator=(Image_Shmdata&& g) noexcept
        {
            if (this != &g)
            {
                _filename = g._filename;
                _reader = g._reader;
                _writer = g._writer;
    
                _writerSpec = g._writerSpec;
                _writerInputSize = g._writerInputSize;
                _writerStartTime = g._writerStartTime;
                _writerBuffer.swap(g._writerBuffer);
    
                _readerBuffer.swap(g._readerBuffer);
                _inputDataType = g._inputDataType;
                _bpp = g._bpp;
                _width = g._width;
                _height = g._height;
                _red = g._red;
                _green = g._green;
                _blue = g._blue;
                _channels = g._channels;
                _isYUV = g._isYUV;
                _is420 = g._is420;
            }
            return *this;
        }

        /**
         * Set the path to read from
         */
        bool read(const std::string& filename);

        /**
         * Write an image to the specified path
         */
        bool write(const oiio::ImageBuf& img, const std::string& filename);

    private:
        std::string _filename;
        shmdata_any_reader_t* _reader {nullptr};
        shmdata_any_writer_t* _writer {nullptr};

        oiio::ImageSpec _writerSpec;
        int _writerInputSize {0};
        unsigned long long _writerStartTime;
        oiio::ImageBuf _writerBuffer;

        oiio::ImageBuf _readerBuffer;
        std::string _inputDataType {""};
        int _bpp {0};
        int _width {0};
        int _height {0};
        int _red {0};
        int _green {0};
        int _blue {0};
        int _channels {0};
        bool _isHap {false};
        bool _isYUV {false};
        bool _is420 {false};

        // Hap specific attributes
        unsigned int _textureFormat {0};

        /**
         * Compute some LUT (currently only the YCbCr to RGB one)
         */
        void computeLUT();

        /**
         * Initialize the shm writer according to the spec
         */
        bool initShmWriter(const oiio::ImageSpec& spec, const std::string& filename);

        /**
         * Shmdata callback
         */
        static void onData(shmdata_any_reader_t* reader, void* shmbuf, void* data, int data_size, unsigned long long timestamp,
            const char* type_description, void* user_data);
        
        /**
         * Read Hap compressed images
         */
        static void readHapFrame(Image_Shmdata* ctx, void* shmbuf, void* data, int data_size);

        /**
         * Read uncompressed RGB or YUV images
         */
        static void readUncompressedFrame(Image_Shmdata* ctx, void* shmbuf, void* data, int data_size);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Image_Shmdata> Image_ShmdataPtr;

/**
 * Utility function to clamp quickly a value
 */
inline int clamp(int v, int a, int b) {return v < a ? a : v > b ? b : v;}

} // end of namespace

#endif // SPLASH_IMAGE_SHMDATA_H
