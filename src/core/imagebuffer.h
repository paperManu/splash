/*
 * Copyright (C) 2016 Emmanuel Durand
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
 * @imagebuffer.h
 * The ImageBufferBuffer and ImageBufferBufferSpec classes
 */

#ifndef SPLASH_IMAGEBUFFER_H
#define SPLASH_IMAGEBUFFER_H

#include <chrono>
#include <memory>
#include <mutex>

#include "./config.h"

#include "./core/coretypes.h"

namespace Splash
{

/*************/
class ImageBufferSpec
{
  public:
    enum class Type : uint32_t
    {
        UINT8 = 1,
        UINT16 = 2,
        FLOAT = 4
    };

    /**
     * \brief Constructor
     */
    ImageBufferSpec(){};

    /**
     * \brief Constructor
     * \param w Width
     * \param h Height
     * \param c Channel count
     * \param b Bit per pixej
     * \param f Format as a string
     */
    ImageBufferSpec(unsigned int w, unsigned int h, unsigned int c, uint8_t b, ImageBufferSpec::Type t = Type::UINT8, const std::string& f = "RGBA")
        : width(w)
        , height(h)
        , channels(c)
        , bpp(b)
        , type(t)
    {
        if (f.empty())
        {
            switch (c)
            {
            default:
            case 1:
                format = "R";
                break;
            case 2:
                format = "RG";
                break;
            case 3:
                format = "RGB";
                break;
            case 4:
                format = "RGBA";
                break;
            }
        }
        else
        {
            format = f;
        }
    }

    uint32_t width{0};
    uint32_t height{0};
    uint32_t channels{0};
    uint8_t bpp{0};
    ImageBufferSpec::Type type{Type::UINT8};
    std::string format{};
    bool videoFrame{true};

    inline bool operator==(const ImageBufferSpec& spec) const
    {
        if (width != spec.width)
            return false;
        if (height != spec.height)
            return false;
        if (channels != spec.channels)
            return false;
        if (bpp != spec.bpp)
            return false;
        if (type != spec.type)
            return false;
        if (format != spec.format)
            return false;

        return true;
    }

    inline bool operator!=(const ImageBufferSpec& spec) const { return !(*this == spec); }

    /**
     * \brief Convert the spec to a string
     * \return Return a string representation of the spec
     */
    std::string to_string() const;

    /**
     * \brief Update from a spec string
     * \param spec Spec string
     */
    void from_string(const std::string& spec);

    /**
     * \brief Get channel size in bytes
     * \return Return channel size
     */
    int pixelBytes() const { return bpp / 8; }

    /**
     * \brief Get image size in bytes
     * \return Return image size
     */
    int rawSize() const { return pixelBytes() * width * height; }
};

/*************/
class ImageBuffer
{
  public:
    /**
     * \brief Constructor
     */
    ImageBuffer() = default;

    /**
     * \brief Constructor
     * \param spec Image spec
     * \param data Pointer to initial data
     * \param map Use the data pointer as the buffer for this ImageBuffer
     */
    ImageBuffer(const ImageBufferSpec& spec, char* data = nullptr, bool map = false);

    /**
     * \brief Destructor
     */
    ~ImageBuffer() = default;

    ImageBuffer(const ImageBuffer& i) = default;
    ImageBuffer(ImageBuffer&& i) = default;
    ImageBuffer& operator=(const ImageBuffer& i) = default;
    ImageBuffer& operator=(ImageBuffer&& i) = default;

    /**
     * \brief Return a pointer to the image data
     * \return Return a pointer to the data
     */
    char* data() const { return _mappedBuffer ? _mappedBuffer : _buffer.data(); }

    /**
     * \brief Get the image spec
     * \return Return image spec
     */
    ImageBufferSpec getSpec() const { return _spec; }

    /**
     * \brief Get the image buffer size
     * \return Return the size
     */
    size_t getSize() const { return _mappedBuffer ? _spec.width * _spec.height * _spec.pixelBytes() : _buffer.size(); }

    /**
     * \brief Fill all channels with the given value
     * \param value Value to fill the image with
     */
    void zero();

    /**
     * \brief Set the inner raw buffer, to use with caution, its size must match the spec
     * \param buffer Buffer to use as inner buffer
     */
    void setRawBuffer(ResizableArray<char>&& buffer)
    {
        if (!_mappedBuffer)
            _buffer = buffer;
    }

  private:
    ImageBufferSpec _spec{};
    ResizableArray<char> _buffer{};
    char* _mappedBuffer{nullptr};
};

} // namespace Splash

#endif // SPLASH_IMAGEBUFFER_H
