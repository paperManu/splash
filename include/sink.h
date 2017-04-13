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

#include <future>
#include <memory>
#include <mutex>

#include "./config.h"

#include "./base_object.h"
#include "./attribute.h"
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
    Sink(RootObject* root);

    /**
     * Destructor
     */
    virtual ~Sink() override;

    /**
     * Try to link the given BaseObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(const std::shared_ptr<BaseObject>& obj);

    /**
     * Try to unlink the given BaseObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(const std::shared_ptr<BaseObject>& obj);

    /**
     * Update the inner buffer of the sink
     */
    void update() override;

    /**
     * Send the inner buffer to the sink's output
     */
    void render() override;

  protected:
    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();

  private:
    std::shared_ptr<Texture> _inputTexture{nullptr};
    ImageBufferSpec _spec{};
    ImageBuffer _image{};
    std::mutex _lockPixels;

    bool _opened{false}; //!< If true, the sink lets frames through
    uint32_t _period{0}; //!< Minimum time between consecutive frames

    uint64_t _lastFrameTiming{0};
    uint32_t _pboCount{3};
    std::vector<GLuint> _pbos{};
    int _pboWriteIndex{0};
    GLubyte* _mappedPixels{nullptr};

    /**
     * Class to be implemented to copy the _mappedPixels somewhere
     */
    virtual void handlePixels(const char* pixels, const ImageBufferSpec& spec) = 0;

    /**
     * \brief Update the pbos according to the parameters
     * \param width Width
     * \param height Height
     * \param bytes Bytes per pixel
     */
    void updatePbos(int width, int height, int bytes);
};

} // end of namespace

#endif
