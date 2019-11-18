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
 * @image_opencv.h
 * The Image_OpenCV class
 */

#ifndef SPLASH_IMAGE_OPENCV_H
#define SPLASH_IMAGE_OPENCV_H

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./image/image.h"

namespace cv
{
class VideoCapture;
}

namespace Splash
{

class Image_OpenCV : public Image
{
  public:
    /**
     * Constructor
     */
    Image_OpenCV(RootObject* root);

    /**
     * Destructor
     */
    ~Image_OpenCV() final;

    /**
     * No copy, but some move constructors
     */
    Image_OpenCV(const Image_OpenCV&) = delete;
    Image_OpenCV& operator=(const Image_OpenCV&) = delete;

    /**
     * Set the path to read from
     */
    bool read(const std::string& filename) final;

  private:
    int _inputIndex{-1};
    unsigned int _width{640};
    unsigned int _height{480};
    float _framerate{60.0};
    float _exposure{10000.f}; //!< Exposure time in us

    std::string _cvOptions{};
    bool _cvOptionsUpdated{false};

    bool _capturing{false};
    std::thread _readLoopThread;
    std::atomic_bool _continueReading{false};
    ImageBuffer _readBuffer;

    /**
     * Base init for the class
     */
    void init();

    /**
     * Parse OpenCV options
     * \param options Options to parse
     * \return Return a map of cv::CAP_PROP indices with their value as a double
     */
    std::map<int, double> parseCVOptions(const std::string& options);

    /**
     * Input read loop
     */
    void readLoop();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_IMAGE_OPENCV_H
