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

#ifndef SPLASH_IMAGE_NDI_H
#define SPLASH_IMAGE_NDI_H

#include "./image/image.h"

#include <memory>

#include <spawn.h>

#include "./image/image_shmdata.h"
#include "./utils/subprocess.h"

namespace Splash
{

class Image_NDI final : public Image
{
  public:
    /**
     * Constructor
     */
    explicit Image_NDI(RootObject* root);

    /**
     * Destructor
     */
    ~Image_NDI() final;

    /**
     * Constructors/operators
     */
    Image_NDI(const Image_NDI&) = delete;
    Image_NDI& operator=(const Image_NDI&) = delete;
    Image_NDI(const Image_NDI&&) = delete;
    Image_NDI& operator=(const Image_NDI&&) = delete;

    /**
     * Set the NDI server to connect to
     * \param sourceName NDI source to connect to
     * \return Return true the NDI client was set successfully
     */
    bool read(const std::string& sourceName) final;

    /**
     * Update the content of the image
     * Image is double buffered, so this has to be called after
     * any new buffer is set for changes to be effective
     */
    void update() final;

  private:
    Image_Shmdata _shmdata;
    std::unique_ptr<Utils::Subprocess> _subprocess{nullptr};

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif
