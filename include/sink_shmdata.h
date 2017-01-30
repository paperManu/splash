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
 * @sink_shmdata.h
 * The Sink_Shmdata class, sending the connected object to shmdata
 */

#ifndef SPLASH_SINK_SHMDATA_H
#define SPLASH_SINK_SHMDATA_H

#include <shmdata/writer.hpp>

#include "./osUtils.h"
#include "./sink.h"

namespace Splash
{

class Sink_Shmdata : public Sink
{
  public:
    /**
     * Constructor
     */
    Sink_Shmdata(std::weak_ptr<RootObject> root);

  private:
    std::string _path{"/tmp/splash_sink"};
    std::string _caps{"application/x-raw"};
    Utils::ConsoleLogger _logger;
    std::unique_ptr<shmdata::Writer> _writer{nullptr};
    ImageBufferSpec _previousSpec{};

    /**
     * Class to be implemented to copy the _mappedPixels somewhere
     */
    void handlePixels(const char* pixels, ImageBufferSpec spec);

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif
