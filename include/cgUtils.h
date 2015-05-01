/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @cgUtils.h
 * Some useful classes for image / pixel manipulation
 */

#ifndef SPLASH_CGUTILS_H
#define SPLASH_CGUTILS_H

#include <vector>
#include <hap.h>

#include "config.h"
#include "coretypes.h"
#include "log.h"

namespace Splash
{

/*************/
struct RgbValue
{
    RgbValue() {};
    RgbValue(Values v)
    {
        if (v.size() != 3)
            return;
        r = v[0].asFloat();
        g = v[1].asFloat();
        b = v[2].asFloat();
    }
    RgbValue(std::vector<float> v)
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

    RgbValue operator*(const RgbValue c) const 
    {
        RgbValue tmp = *this;
        tmp.r *= c.r;
        tmp.g *= c.g;
        tmp.b *= c.b;
        return tmp;
    }

    RgbValue operator/(const RgbValue c) const 
    {
        RgbValue tmp = *this;
        tmp.r /= c.r;
        tmp.g /= c.g;
        tmp.b /= c.b;
        return tmp;
    }

    RgbValue operator+(const RgbValue c) const 
    {
        RgbValue tmp = *this;
        tmp.r += c.r;
        tmp.g += c.g;
        tmp.b += c.b;
        return tmp;
    }

    // Get the luminance, considering a sRGB linearized color space
    float luminance() const 
    {
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

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

    float r {0.f};
    float g {0.f};
    float b {0.f};
};

/*************/
// Decode a Hap frame
// If out is null, only sets the format
inline bool hapDecodeFrame(void* in, unsigned int inSize, void* out, unsigned int outSize, std::string& format)
{
    // We are using kind of a hack to store a DXT compressed image in an oiio::ImageBuf
    // First, we check the texture format type
    unsigned int textureFormat = 0;
    if (HapGetFrameTextureFormat(in, inSize, &textureFormat) != HapResult_No_Error)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - Unknown texture format. Frame discarded" << Log::endl;
        return false;
    }

    if (textureFormat == HapTextureFormat_RGB_DXT1)
        format = "RGB_DXT1";
    else if (textureFormat == HapTextureFormat_RGBA_DXT5)
        format = "RGB_DXT5";
    else if (textureFormat == HapTextureFormat_YCoCg_DXT5)
        format = "YCoCg_DXT5";
    else
        return false;

    if (out == nullptr)
        return true;

    unsigned long bytesUsed = 0;
    if (HapDecode(in, inSize, NULL, NULL, out, outSize, &bytesUsed, &textureFormat) != HapResult_No_Error)
    {
        SLog::log << Log::WARNING << __FUNCTION__ << " - An error occured while decoding frame" << Log::endl;
        return false;
    }

    return true;
}

} // end of namespace

#endif
