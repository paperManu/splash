/*
 * Copyright (C) 2017 Emmanuel Durand
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
 * @sink.h
 * The Sink base class
 */

#ifndef SPLASH_SINK_H
#define SPLASH_SINK_H

#include <memory>
#include <mutex>

#include "./config.h"

#include "./basetypes.h"
#include "./coretypes.h"
#include "./texture.h"

namespace Splash
{

class Sink : public BaseObject
{
  public:
    /**
     * Constructor
     */
    Sink(std::weak_ptr<RootObject> root);

    /**
     * Destructor
     */
    virtual ~Sink();

    /**
     * Try to link the given BaseObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(std::shared_ptr<BaseObject> obj);

    /**
     * Try to unlink the given BaseObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(std::shared_ptr<BaseObject> obj);

    /**
     * Update the inner buffer of the sink
     */
    void update();

    /**
     * Send the inner buffer to the sink's output
     */
    void render();

  private:
    std::shared_ptr<Texture> _inputTexture{nullptr};
    ImageBufferSpec _spec{};
    ImageBuffer _image{};
    std::mutex _lockPixels;
    std::thread _handlePixelsThread{};

    GLuint _pbos[2];
    int _pboWriteIndex{0};
    GLubyte* _mappedPixels{nullptr};

    /**
     * Class to be implemented to copy the _mappedPixels somewhere
     */
    virtual void handlePixels(const char* pixels, ImageBufferSpec spec) = 0;

    /**
     * \brief Update the pbos according to the parameters
     * \param width Width
     * \param height Height
     * \param bytes Bytes per pixel
     */
    void updatePbos(int width, int height, int bytes);

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif
