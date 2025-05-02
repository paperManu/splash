/*
 * Copyright (C) 2021 Splash authors
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
 * @image_ndi.h
 * The Image_NDI class
 */

#ifndef SPLASH_IMAGE_NDI2_H
#define SPLASH_IMAGE_NDI2_H

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <Processing.NDI.Lib.h>

#include "./image/image.h"

namespace Splash
{

class Image_NDI final : public Image
{
  public:
    /**
     * Constructor
     */
    explicit Image_NDI(RootObject* root);
    ~Image_NDI() final;
    Image_NDI(const Image_NDI&) = delete;
    Image_NDI& operator=(const Image_NDI&) = delete;
    Image_NDI(const Image_NDI&&) = delete;
    Image_NDI& operator=(const Image_NDI&&) = delete;

    /**
     * Set the NDI server to connect to.
     * \param sourceName NDI source to connect to
     * \return Return true the NDI client was set successfully
     */
    bool read(const std::string& sourceName) final;

  private:
    enum class NDILoadStatus : uint8_t
    {
        NotLoaded,
        Loaded,
        Failed
    };

    static NDILoadStatus _ndiLoaded;
    static NDIlib_v4* _ndiLib;

    static std::mutex _ndiFindMutex;
    static NDIlib_find_instance_t _ndiFind;

    static std::mutex _ndiSourcesMutex;
    static std::vector<NDIlib_source_t> _ndiSources;

    std::string _sourceName = "";
    NDIlib_recv_instance_t _ndiReceiver;
    std::mutex _recvMutex;
    bool _receive = false;
    std::thread _recvThread;

    /**
     * Load the NDI library.
     * \return True if the library has been (or was already) loaded successfully
     */
    static bool loadNDILib();

    /**
     * Connect to an existing NDI source by its name
     * \param name Source name
     * \return True if connection is succesful
     */
    bool connectByName(std::string_view name);

    /**
     * Read the given NDI video frame and put it in and ImageBuffer
     * \param ndiFrame NDI video frame
     * \return An ImageBuffer
     */
    ImageBuffer readNDIVideoFrame(const NDIlib_video_frame_v2_t& ndiFrame);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif
