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

#include "config.h"

#include "basetypes.h"
#include "coretypes.h"
#include "imageBuffer.h"

namespace Splash
{

class Image : public BufferObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Image(std::weak_ptr<RootObject> root);

    /**
     * \brief Constructor
     * \param root Root object
     * \param spec Image specifications
     */
    Image(std::weak_ptr<RootObject> root, ImageBufferSpec spec);

    /**
     * \brief Destructor
     */
    virtual ~Image();

    /**
     * No copy constructor, but a copy operator
     */
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = default;

    /**
     * \brief Get a pointer to the data
     * \return Return a pointer to the data
     */
    const void* data() const;

    /**
     * \brief Lock the image, useful while reading. Use with care Note that only write mutex is needed, as it also disables reading
     */
    void lock() { _writeMutex.lock(); }

    /**
     * \brief Unlock the image
     */
    void unlock() { _writeMutex.unlock(); }

    /**
     * \brief Get the image buffer
     * \return Return the image buffer
     */
    ImageBuffer get() const;

    /**
     * \brief Get the file path
     * \return Return the file path
     */
    std::string getFilepath() const { return _filepath; }

    /**
     * \brief Get the image buffer specs
     * \return Return the image buffer specs
     */
    ImageBufferSpec getSpec() const;

    /**
     * \brief Set the image from an ImageBuffer
     * \param img Image buffer
     */
    void set(const ImageBuffer& img);

    /**
     * \brief Set the image as a empty with the given size / channels / typedesc
     * \param w Width
     * \param h Height
     * \param channels Channel count
     * \param type Channel type
     */
    void set(unsigned int w, unsigned int h, unsigned int channels, ImageBufferSpec::Type type);

    /**
     * \brief Serialize the image
     * \return Return the serialized image
     */
    std::shared_ptr<SerializedObject> serialize() const;

    /**
     * \brief Update the Image from a serialized representation
     * \param obj Serialized image
     * \return Return true if all went well
     */
    bool deserialize(const std::shared_ptr<SerializedObject>& obj);

    /**
     * \brief Set the path to read from
     * \param filename File path
     * \return Return true if all went well
     */
    virtual bool read(const std::string& filename);

    /**
     * Set all pixels in the image to zero
     */
    void zero();

    /**
     * \brief Update the content of the image
     */
    virtual void update();

    /**
     * \brief Write the current buffer to the specified file
     * \param filename File path to write to
     * \return Return true if all went well
     */
    bool write(const std::string& filename);

  protected:
    std::unique_ptr<ImageBuffer> _image;
    std::unique_ptr<ImageBuffer> _bufferImage;
    std::string _filepath;
    bool _flip{false};
    bool _flop{false};
    bool _imageUpdated{false};
    bool _srgb{true};
    bool _benchmark{false};
    bool _worldObject{false};

    void createDefaultImage(); //< Create a default black image
    void createPattern();      //< Create a default pattern

    /**
     * \brief Read the specified image file
     * \param filename File path
     * \return Return true if all went well
     */
    bool readFile(const std::string& filename);

  private:
    // Deserialization is done in this buffer, to avoid realloc
    ImageBuffer _bufferDeserialize;

    /**
     * \brief Base init for the class
     */
    void init();

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_IMAGE_H
