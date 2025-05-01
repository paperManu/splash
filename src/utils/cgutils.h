/*
 * Copyright (C) 2015 Splash authors
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
 * @cgutils.h
 * Some useful classes for image / pixel manipulation
 */

#ifndef SPLASH_CGUTILS_H
#define SPLASH_CGUTILS_H

#include <span>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "./hap.h"

#include "./core/constants.h"

#include "./utils/log.h"

namespace Splash
{

/*************/
// Color stuff
/*************/

/*************/
struct RgbValue
{
    RgbValue(){};
    RgbValue(const Values& v)
    {
        if (v.size() != 3)
            return;
        r = v[0].as<float>();
        g = v[1].as<float>();
        b = v[2].as<float>();
    }
    RgbValue(const std::vector<float>& v)
    {
        if (v.size() != 3)
            return;
        r = v[0];
        g = v[1];
        b = v[2];
    }
    RgbValue(float pr, float pg, float pb)
    {
        r = pr;
        g = pg;
        b = pb;
    }

    float& operator[](unsigned int c)
    {
        if (c == 0 || c > 2)
            return r;
        else if (c == 1)
            return g;
        else
            return b;
    }

    RgbValue operator*(const float v) const
    {
        RgbValue tmp = *this;
        tmp.r *= v;
        tmp.g *= v;
        tmp.b *= v;
        return tmp;
    }

    RgbValue operator/(const float v) const
    {
        RgbValue tmp = *this;
        tmp.r /= v;
        tmp.g /= v;
        tmp.b /= v;
        return tmp;
    }

    RgbValue operator*(const RgbValue& c) const
    {
        RgbValue tmp = *this;
        tmp.r *= c.r;
        tmp.g *= c.g;
        tmp.b *= c.b;
        return tmp;
    }

    RgbValue operator/(const RgbValue& c) const
    {
        RgbValue tmp = *this;
        tmp.r /= c.r;
        tmp.g /= c.g;
        tmp.b /= c.b;
        return tmp;
    }

    RgbValue operator+(const RgbValue& c) const
    {
        RgbValue tmp = *this;
        tmp.r += c.r;
        tmp.g += c.g;
        tmp.b += c.b;
        return tmp;
    }

    void operator+=(const RgbValue& c) { *this = *this + c; }

    void operator/=(const float v) { *this = *this / v; }

    // Get the luminance, considering a sRGB linearized color space
    float luminance() const { return 0.2126 * r + 0.7152 * g + 0.0722 * b; }

    // Normalizes in a colorspace manner, i.e so that max value = 1.f
    RgbValue& normalize()
    {
        float max = std::max(std::max(r, g), b);
        r /= max;
        g /= max;
        b /= max;
        return *this;
    }

    void set(int i, float v)
    {
        if (i == 0)
            r = v;
        else if (i == 1)
            g = v;
        else if (i == 2)
            b = v;
    }

    float r{0.f};
    float g{0.f};
    float b{0.f};
};

/**
 * Convert to input YUV I420 planar buffer to an interlaced UYVY
 * Output buffer must have the right size, hence 16bits per pixel
 *
 * \param input Input I420 planar buffer
 * \param output Output UYVY planar buffer
 * \param width Image width
 * \param height Image height
 */
inline void cvtI420toUYVY(const std::span<uint8_t>& input, std::span<uint8_t>& output, uint32_t width, uint32_t height)
{
    const auto data = input.data();
    const auto Y = data;
    const auto U = data + width * height;
    const auto V = data + width * height * 5 / 4;
    auto pixels = output.data();

    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; x += 2)
        {
            pixels[(x + y * width) * 2 + 0] = U[(x / 2) + (y / 2) * (width / 2)];
            pixels[(x + y * width) * 2 + 1] = Y[x + y * width];
            pixels[(x + y * width) * 2 + 2] = V[(x / 2) + (y / 2) * (width / 2)];
            pixels[(x + y * width) * 2 + 3] = Y[x + y * width + 1];
        }
    }
}

/**
 * Convert to input YUV YV12 planar buffer to an interlaced UYVY
 * Output buffer must have the right size, hence 16bits per pixel
 *
 * \param input Input YV12 planar buffer
 * \param output Output UYVY planar buffer
 * \param width Image width
 * \param height Image height
 */
inline void cvtYV12toUYVY(const std::span<uint8_t>& input, std::span<uint8_t>& output, uint32_t width, uint32_t height)
{
    const auto data = input.data();
    const auto Y = data;
    const auto V = data + width * height;
    const auto U = data + width * height * 5 / 4;
    auto pixels = output.data();

    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; x += 2)
        {
            pixels[(x + y * width) * 2 + 0] = U[(x / 2) + (y / 2) * (width / 2)];
            pixels[(x + y * width) * 2 + 1] = Y[x + y * width];
            pixels[(x + y * width) * 2 + 2] = V[(x / 2) + (y / 2) * (width / 2)];
            pixels[(x + y * width) * 2 + 3] = Y[x + y * width + 1];
        }
    }
}

/**
 * Convert to input YUV NV12 planar buffer to an interlaced UYVY
 * Output buffer must have the right size, hence 16bits per pixel
 *
 * \param input Input NV12 planar buffer
 * \param output Output UYVY planar buffer
 * \param width Image width
 * \param height Image height
 */
inline void cvtNV12toUYVY(const std::span<uint8_t>& input, std::span<uint8_t>& output, uint32_t width, uint32_t height)
{
    const auto data = input.data();
    const auto Y = data;
    const auto UV = data + width * height;
    auto pixels = output.data();

    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; x += 2)
        {
            pixels[(x + y * width) * 2 + 0] = UV[x + (y / 2) * width];
            pixels[(x + y * width) * 2 + 1] = Y[x + y * width];
            pixels[(x + y * width) * 2 + 2] = UV[x + (y / 2) * width + 1];
            pixels[(x + y * width) * 2 + 3] = Y[x + y * width + 1];
        }
    }
}

/**
 * Convert to input YUV P216 planar buffer to an interlaced UYVY
 * Output buffer must have the right size, hence 16bits per pixel
 * This conversion scales all values by 256.
 *
 * \param input Input P216 planar buffer
 * \param output Output UYVY planar buffer
 * \param width Image width
 * \param height Image height
 */
inline void cvtP216toUYVY(const std::span<uint16_t>& input, std::span<uint8_t>& output)
{
    for (uint32_t p = 0; p < input.size(); ++p)
        output[p] = input[p] >> 8;
}

/**
 * Get the color balance (r/g and b/g) from a black body temperature
 */
inline glm::vec2 colorBalanceFromTemperature(float temp)
{
    using glm::log;
    using glm::max;
    using glm::min;
    using glm::pow;

    glm::dvec3 c;
    float t = temp / 100.0;
    if (t <= 66.0)
        c.r = 255.0;
    else
    {
        c.r = t - 60.0;
        c.r = 329.698727466 * pow(c.r, -0.1332047592);
        c.r = max(0.0, min(c.r, 255.0));
    }

    if (t <= 66)
    {
        c.g = t;
        c.g = 99.4708025861 * log(c.g) - 161.1195681661;
        c.g = max(0.0, min(c.g, 255.0));
    }
    else
    {
        c.g = t - 60.0;
        c.g = 288.1221695283 * pow(c.g, -0.0755148492);
        c.g = max(0.0, min(c.g, 255.0));
    }

    if (t >= 66)
        c.b = 255.0;
    else
    {
        if (t <= 19)
            c.b = 0.0;
        else
        {
            c.b = t - 10.0;
            c.b = 138.5177312231 * log(c.b) - 305.0447927307;
            c.b = max(0.0, min(c.b, 255.0));
        }
    }

    glm::vec2 colorBalance;
    colorBalance.x = c.r / c.g;
    colorBalance.y = c.b / c.g;

    return colorBalance;
}

/**
 * Get the projection matrix given the fov and shift (0.5 meaning no shift, 0.0 and 1.0 meaning 100% on one direction or the other)
 * \param fov Field of view, in degrees
 * \param cx Center shift along X
 * \param cy Center shift along Y
 * \return Return the projection matrix
 */
inline glm::dmat4 getProjectionMatrix(float fov, float near, float far, float width, float height, float cx, float cy)
{
    double l, r, t, b, n, f;
    n = near;
    f = far;
    // Up and down
    double tTemp = n * tan(fov * M_PI / 360.0);
    t = tTemp - (cy - 0.5) * 2.0 * tTemp;
    b = -tTemp - (cy - 0.5) * 2.0 * tTemp;
    // Left and right
    double rTemp = tTemp * width / height;
    double lTemp = -tTemp * width / height;
    r = rTemp - (cx - 0.5) * (rTemp - lTemp);
    l = lTemp - (cx - 0.5) * (rTemp - lTemp);

    return glm::frustum(l, r, b, t, n, f);
}

/*************/
// HAP
/*************/
// Hap chunk callback
void hapDecodeCallback(HapDecodeWorkFunction func, void* p, unsigned int count, void* info);
// Decode a Hap frame
// If out is null, only sets the format
bool hapDecodeFrame(void* in, unsigned int inSize, void* out, unsigned int outSize, std::string& format);

} // namespace Splash

#endif
