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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @image_shmdata.h
 * The Image_Shmdata class
 */

#ifndef SPLASH_IMAGE_SHMDATA_H
#define SPLASH_IMAGE_SHMDATA_H

#include <shmdata/console-logger.hpp>
#include <shmdata/follower.hpp>

#include "config.h"

#include "image.h"
#include "osUtils.h"

namespace Splash
{

class Image_Shmdata : public Image
{
    public:
        /**
         * Constructor
         */
        Image_Shmdata(std::weak_ptr<RootObject> root);

        /**
         * Destructor
         */
        ~Image_Shmdata();

        /**
         * No copy constructor, only move
         */
        Image_Shmdata(const Image_Shmdata&) = delete;
        Image_Shmdata& operator=(const Image_Shmdata&) = delete;

        /**
         * Set the path to read from
         */
        bool read(const std::string& filename);

    private:
        Utils::ConsoleLogger _logger;
        std::unique_ptr<shmdata::Follower> _reader {nullptr};

        ImageBuffer _readerBuffer;
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
        bool _is422 {false};

        // Hap specific attributes
        std::string _textureFormat {""};

        /**
         * Compute some LUT (currently only the YCbCr to RGB one)
         */
        void computeLUT();

        /**
         * Base init for the class
         */
        void init();

        /**
         * \brief Callback called when receiving a new caps
         * \param dataType String holding the data type
         */
        void onCaps(const std::string& dataType);

        /**
         * \brief Callback called when receiving a new frame
         * \param data Content of the frame
         * \param data_size Size of the frame
         * \param user_data User data
         */
        void onData(void* data, int data_size);
        
        /**
         * Read Hap compressed images
         */
        void readHapFrame(void* data, int data_size);

        /**
         * Read uncompressed RGB or YUV images
         */
        void readUncompressedFrame(void* data, int data_size);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

/**
 * Utility function to clamp quickly a value
 */
inline int clamp(int v, int a, int b) {return v < a ? a : v > b ? b : v;}

} // end of namespace

#endif // SPLASH_IMAGE_SHMDATA_H
