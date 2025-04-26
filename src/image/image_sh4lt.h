/*
 * Copyright (C) 2024 Splash authors
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
 * @image_sh4lt.h
 * The Image_Sh4lt class
 */

#ifndef SPLASH_IMAGE_SH4LT_H
#define SPLASH_IMAGE_SH4LT_H

#include <map>
#include <mutex>
#include <set>

#include <sh4lt/follower.hpp>
#include <sh4lt/monitor/monitor.hpp>

#include "./core/constants.h"

#include "./image/image.h"
#include "./utils/osutils.h"

namespace Splash
{

class Image_Sh4lt final : public Image
{
  public:
    /**
     * Constructor
     */
    explicit Image_Sh4lt(RootObject* root);

    /**
     * Destructor
     */
    ~Image_Sh4lt() final;

    /**
     * Constructors/operators
     */
    Image_Sh4lt(const Image_Sh4lt&) = delete;
    Image_Sh4lt& operator=(const Image_Sh4lt&) = delete;
    Image_Sh4lt(Image_Sh4lt&&) = delete;
    Image_Sh4lt& operator=(Image_Sh4lt&&) = delete;

    /**
     * Set the path to read from
     */
    bool read(const std::string& filename) final;

  private:
    static const uint32_t _sh4ltCopyThreads = 2;
    std::string _label{""};
    std::string _group{sh4lt::ShType::default_group()};
    std::unique_ptr<sh4lt::Follower> _reader{nullptr};

    std::unique_ptr<sh4lt::monitor::Monitor> _monitor{nullptr};
    std::mutex _monitorMutex;
    std::set<std::string> _groups;              //!< All available groups
    std::set<std::string> _labels;              //!< Available labels for current _group
    std::map<std::string, std::string> _medias; //!< Media info for all of _labels

    ImageBuffer _readerBuffer;
    uint32_t _bpp{0};
    uint32_t _width{0};
    uint32_t _height{0};
    uint32_t _red{0};
    uint32_t _green{0};
    uint32_t _blue{0};
    uint32_t _channels{0};
    bool _isVideo{false};
    bool _isDepth{false};
    bool _isHap{false};
    bool _isYUV{false};
    bool _is420{false};
    bool _is422{false};

    // Hap specific attributes
    std::string _textureFormat{""};

    /**
     * Compute some LUT (currently only the YCbCr to RGB one)
     */
    void computeLUT();

    /**
     * Callback called when receiving a new caps
     * \param dataType String holding the data type
     */
    void onShType(const sh4lt::ShType& dataType);

    /**
     * Construct the internal Sh4lt using label and group
     **/
    bool readByLabel();

    /**
     * Callback called when receiving a new frame
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

    /**
     * Small function to work around a bug in GCC's libstdc++
     *
     * \param str The string that need extra parenthesis removal
     */
    static void removeExtraParenthesis(std::string& str);
};

} // namespace Splash

#endif // SPLASH_IMAGE_SH4LT_H
