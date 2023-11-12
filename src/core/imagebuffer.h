/*
 * Copyright (C) 2016 Splash authors
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
#include <string>

#include "./core/constants.h"

#include "./utils/resizable_array.h"

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
     * Constructor
     */
    ImageBufferSpec(){};

    /**
     * Constructor
     * \param stringSpec Spec as string
     */
    ImageBufferSpec(const std::string& stringSpec) { from_string(stringSpec); }

    /**
     * Constructor
     * \param w Width
     * \param h Height
     * \param c Channel count
     * \param b Bit per pixej
     * \param t Component type
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
    int64_t timestamp{-1};

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

    inline bool operator!=(const ImageBufferSpec& spec) const { return !operator==(spec); }

    /**
     * Convert the spec to a string
     * \return Return a string representation of the spec
     */
    std::string to_string() const;

    /**
     * Update from a spec string
     * \param spec Spec string
     */
    void from_string(const std::string& spec);

    /**
     * Get channel size in bytes
     * \return Return channel size
     */
    int pixelBytes() const { return bpp / 8; }

    /**
     * Get image size in bytes
     * \return Return image size
     */
    int rawSize() const { return pixelBytes() * width * height; }
};

/*************/
class ImageBuffer
{
  public:
    /**
     * Constructor
     */
    ImageBuffer() = default;

    /**
     * Constructor
     * \param spec Image spec
     * \param data Pointer to initial data
     * \param map Use the data pointer as the buffer for this ImageBuffer
     */
    ImageBuffer(const ImageBufferSpec& spec, uint8_t* data = nullptr, bool map = false);

    /**
     * Destructor
     */
    ~ImageBuffer() = default;

    ImageBuffer(const ImageBuffer& i) = default;
    ImageBuffer(ImageBuffer&& i) = default;
    ImageBuffer& operator=(const ImageBuffer& i) = default;
    ImageBuffer& operator=(ImageBuffer&& i) = default;

    /**
     * Return a pointer to the image data
     * \return Return a pointer to the data
     */
    uint8_t* data() { return _mappedBuffer ? _mappedBuffer : _buffer.data(); }
    const uint8_t* data() const { return _mappedBuffer ? _mappedBuffer : _buffer.data(); }

    /**
     * Get a const reference to the inner buffer
     * \return Return a const ref to the inner buffer
     */
    const ResizableArray<uint8_t>& getRawBuffer() const { return _buffer; }

    /**
     * Get the image spec
     * \return Return image spec
     */
    ImageBufferSpec& getSpec() { return _spec; }
    const ImageBufferSpec& getSpec() const { return _spec; }

    /**
     * Return true if the image buffer is empty
     * \return Return true if empty
     */
    bool empty() const { return getSize() == 0; }

    /**
     * Get the name of the image buffer
     * \return Return the name
     */
    std::string getName() const { return _name; }

    /**
     * Get the image buffer size
     * \return Return the size
     */
    size_t getSize() const { return _mappedBuffer ? _spec.width * _spec.height * _spec.pixelBytes() : _buffer.size(); }

    /**
     * Set the name of the image buffer
     * \param name Name for the buffer
     */
    void setName(const std::string& name) { _name = name; }

    /**
     * Fill all channels with the given value
     * \param value Value to fill the image with
     */
    void zero();

    /**
     * Set the inner raw buffer, to use with caution, its size must match the spec
     * \param buffer Buffer to use as inner buffer
     */
    void setRawBuffer(ResizableArray<uint8_t>&& buffer)
    {
        if (!_mappedBuffer)
            _buffer = buffer;
    }

  private:
    std::string _name{};
    ImageBufferSpec _spec{};
    ResizableArray<uint8_t> _buffer;
    uint8_t* _mappedBuffer{nullptr};
};

} // namespace Splash

#endif // SPLASH_IMAGEBUFFER_H
