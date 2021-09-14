/*
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

#include <iostream>
#include <vector>

#include <doctest.h>

#include "./core/imagebuffer.h"
#include "./core/serializer.h"
#include "./core/serialize/serialize_imagebuffer.h"

using namespace Splash;

/*************/
TEST_CASE("Testing ImageBuffer serialization")
{
    ImageBufferSpec spec(256, 256, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
    ImageBuffer imageBuffer(spec);
    imageBuffer.setName("someName");

    for (size_t y = 0; y < spec.height; ++y)
        for (size_t x = 0; x < spec.width; ++x)
            imageBuffer.data()[x + y * spec.width] = 42;

    std::vector<uint8_t> buffer;
    Serial::serialize(imageBuffer, buffer);

    auto deserializedImage = Serial::deserialize<ImageBuffer>(buffer);
    CHECK_EQ(spec, deserializedImage.getSpec());

    bool isSame = true;
    for (size_t y = 0; y < spec.height; ++y)
        for (size_t x = 0; x < spec.width; ++x)
            isSame = (imageBuffer.data()[x + y * spec.width] == deserializedImage.data()[x + y * spec.width]);
    CHECK(isSame);
}
