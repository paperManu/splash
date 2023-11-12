/*
 * Copyright (C) 2013 Splash authors
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
 * @image.h
 * The Image class
 */

#ifndef SPLASH_IMAGE_H
#define SPLASH_IMAGE_H

#include <chrono>
#include <mutex>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./core/imagebuffer.h"
#include "./core/root_object.h"
#include "./utils/cgutils.h"

namespace Splash
{

class Image : public BufferObject
{
  public:
    /**
     * Constructor
     * \param root Root object
     * \param spec Image specifications
     */
    Image(RootObject* root, const std::optional<ImageBufferSpec> spec = std::nullopt);

    /**
     * Destructor
     */
    virtual ~Image() override;

    /**
     * Constructors/operators
     */
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;

    /**
     * Get a pointer to the data
     * \return Return a pointer to the data
     */
    const void* data() const;

    /**
     * Get the image buffer
     * \return Return the image buffer
     */
    ImageBuffer get() const;

    /**
     * Get the file path
     * \return Return the file path
     */
    std::string getFilepath() const { return _filepath; }

    /**
     * Get the image buffer specs
     * \return Return the image buffer specs
     */
    ImageBufferSpec getSpec() const;

    /**
     * Get the timestamp for the current image
     * \return Return the timestamp
     */
    virtual int64_t getTimestamp() const final
    {
        std::shared_lock<std::shared_mutex> readLock(_readMutex);
        return _image ? _image->getSpec().timestamp : 0;
    }

    /**
     * Set the timestamp
     * \param timestamp Timestamp, in us
     */
    void setTimestamp(int64_t timestamp) override
    {
        std::lock_guard<std::shared_mutex> readLock(_readMutex);
        _image->getSpec().timestamp = timestamp;
    }

    /**
     * Set the image from an ImageBuffer
     * \param img Image buffer
     */
    void set(const ImageBuffer& img);

    /**
     * Set the image as a empty with the given size / channels / typedesc
     * \param w Width
     * \param h Height
     * \param channels Channel count
     * \param type Channel type
     */
    void set(unsigned int w, unsigned int h, unsigned int channels, ImageBufferSpec::Type type);

    /**
     * Serialize the image
     * \return Return the serialized image
     */
    SerializedObject serialize() const override;

    /**
     * Update the Image from a serialized representation
     * \param obj Serialized image
     * \return Return true if all went well
     */
    bool deserialize(SerializedObject&& obj) override;

    /**
     * Set the path to read from
     * \param filename File path
     * \return Return true if all went well
     */
    virtual bool read(const std::string& filename);

    /**
     * Set all pixels in the image to zero
     */
    void zero();

    /**
     * Update the content of the image
     * Image is double buffered, so this has to be called after
     * any new buffer is set for changes to be effective
     */
    virtual void update() override;

    /**
     * Write the current buffer to the specified file
     * \param filename File path to write to
     * \return Return true if all went well
     */
    bool write(const std::string& filename);

    /**
     * Read the RGB value at the specified x and y pixel locations. Assumes the image is RGBA and that each channel occupies one byte.
     * Also assumes that the x and y positions are within the bounds of the current mipmap level that's loaded.
     * Currently this suppors only RGBA 32bpp images
     * \return The RGB value at (x, y)
     */
    RgbValue readPixel(uint x, uint y) const;

  protected:
    std::unique_ptr<ImageBuffer> _image{nullptr};
    std::unique_ptr<ImageBuffer> _bufferImage{nullptr};
    std::string _filepath{""};

    Values _mediaInfo{};
    std::mutex _mediaInfoMutex{};

    bool _flip{false};
    bool _flop{false};
    bool _bufferImageUpdated{false};
    bool _showPattern{false};
    bool _srgb{true};
    bool _benchmark{false};

    void initFromSpec(const ImageBufferSpec& spec); //< Create an unintialized image with the passed spec.
    void createPattern();                           //< Create a default pattern

    /**
     * Update the _mediaInfo member
     */
    void updateMediaInfo();

    /**
     * Read the specified image file
     * \param filename File path
     * \return Return true if all went well
     */
    bool readFile(const std::string& filename);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

    /**
     * Update the timestamp of the object. Also, set the update flag to true.
     * \param timestamp Value to set the timestamp to, -1 to set to the current
     * time
     */
    virtual void updateTimestamp(int64_t timestamp = -1) final;

  private:
    static const uint32_t _imageCopyThreads = 2;
    static const uint32_t _serializedImageHeaderSize = 4096;
    // Deserialization is done in this buffer, to avoid realloc
    ImageBuffer _bufferDeserialize;

    /**
     * Add more media info, to be implemented by derived classes
     */
    virtual void updateMoreMediaInfo(Values& /*mediaInfo*/) {}

    /**
     * Base init for the class
     */
    void init(std::optional<const ImageBufferSpec> spec = std::nullopt);
};

} // namespace Splash

#endif // SPLASH_IMAGE_H
