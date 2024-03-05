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
 * @sink_sh4lt.h
 * The Sink_Sh4lt class, sending the connected object to sh4lt
 */

#ifndef SPLASH_SINK_SH4LT_H
#define SPLASH_SINK_SH4LT_H

#include <sh4lt/writer.hpp>

#include "./sink/sink.h"
#include "./utils/osutils.h"

namespace Splash
{

class Sink_Sh4lt final : public Sink
{
  public:
    /**
     * Constructor
     */
    explicit Sink_Sh4lt(RootObject* root);

  private:
    std::string _group{sh4lt::ShType::default_group()};
    std::string _label{"splash"};
    sh4lt::ShType _shtype{};
    std::unique_ptr<sh4lt::Writer> _writer{nullptr};
    ImageBufferSpec _previousSpec{};
    uint32_t _previousFramerate{0};

    /**
     * Class to be implemented to copy the _mappedPixels somewhere
     * \param pixels Input image
     * \param spec Input image specifications
     */
    void handlePixels(const char* pixels, const ImageBufferSpec& spec) final;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif
