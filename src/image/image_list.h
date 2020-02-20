/*
 * Copyright (C) 2020 Marie-Eve Dumas
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
 * @image_list.h
 * The Image_List class
 */

#ifndef SPLASH_IMAGE_LIST_H
#define SPLASH_IMAGE_LIST_H

#include "./image/image_sequence.h"

namespace Splash
{

class Image_List : public Image_Sequence
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    Image_List(RootObject* root);

    /**
     * No copy constructor
     */
    Image_List(const Image_List&) = delete;
    Image_List& operator=(const Image_List&) = delete;

    /**
     * Set the path to read from. Must be a valid directory
     * \param directory name
     */
    bool read(const std::string& dirname) final;

    /**
     * Obtain a new image
     */
    bool capture() final;

    /**
     * Get the vector of images file name
     */
    const std::vector<std::string> getFileList() const { return _filenameSeq; };

  private:
    std::vector<std::string> _filenameSeq;

    /**
     * Base init for the class
     */
    void init();
};

} // namespace Splash

#endif // SPLASH_IMAGE_LIST_H
