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
 * @splash-check-calibration.cpp
 * A tool to check the calibration of a projection setup
 */

#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#define PIC_DISABLE_OPENGL
#define PIC_DISABLE_QT
#include <piccante.hpp>

#include "cgUtils.h"
#include "log.h"

using namespace Splash;

/*************/
struct Parameters
{
    bool valid {true};
    std::string filename {""};
    unsigned int subdivisions = 0;
};

/*************/
Parameters parseArgs(int argc, char** argv)
{
    using std::string;

    Parameters params;

    // Get params
    for (unsigned int i = 0; i < argc;)
    {
        if (string(argv[i]) == "-f" && i < argc - 1)
        {
            ++i;
            params.filename = string(argv[i]);
        }
        if (string(argv[i]) == "-s" && i < argc - 1)
        {
            ++i;
            params.subdivisions = std::stoi(string(argv[i]));
        }
        ++i;
    }
    
    // Check params
    if (params.filename.find("hdr") == string::npos)
    {
        params.valid = false;
        SLog::log << Log::WARNING << "Please specify a HDR file to process." << Log::endl;
    }

    return params;
}

/*************/
double getStdDev(std::shared_ptr<pic::Image> image, int x = 0, int y = 0, int w = 0, int h = 0)
{
    using std::cout;
    using std::endl;

    if (!image->isValid())
        return 0.0;

    int width = w == 0 ? image->width : std::min(w, image->width);
    int height = h == 0 ? image->height : std::min(h, image->height);

    pic::BBox roi = pic::BBox(x, x+width, y, y+height);
    double totalPixels = width * height;

    double meanValue = 0.0;
    for (int y = roi.y0; y < roi.y1; ++y)
        for (int x = roi.x0; x < roi.x1; ++x)
        {
            float* pixel = (*image)(x, y);
            RgbValue rgb(pixel[0], pixel[1], pixel[2]);
            meanValue += rgb.luminance();
        }
    meanValue /= totalPixels;

    double stdDev = 0.0;
    for (int y = roi.y0; y < roi.y1; ++y)
        for (int x = roi.x0; x < roi.x1; ++x)
        {
            float* pixel = (*image)(x, y);
            RgbValue rgb(pixel[0], pixel[1], pixel[2]);
            double lum = rgb.luminance();
            stdDev += std::pow(lum - meanValue, 2.0);
        }

    stdDev = std::sqrt(stdDev / totalPixels);

    return stdDev;
}

/*************/
std::vector<double> applyMultilevel(std::function<double(std::shared_ptr<pic::Image>, int, int, int, int)> filter,
                                    std::shared_ptr<pic::Image> image, unsigned int levels)
{
    std::vector<double> results;

    for (unsigned int l = 0; l <= levels; ++l)
    {
        int blockWidth = image->width / std::pow(2.0, l);
        int blockHeight = image->height / std::pow(2.0, l);
        
        for (int x = 0; x < image->width; x += blockWidth)
            for (int y = 0; y < image->height; y += blockHeight)
            {
                double result = filter(image, x, y, blockWidth, blockHeight);
                results.push_back(result);
            }
    }

    return results;
}

/*************/
int main(int argc, char** argv)
{
    Parameters params = parseArgs(argc, argv);

    if (!params.valid)
    {
        SLog::log << Log::WARNING << "An error was found in the parameters, exiting." << Log::endl;
        exit(1);
    }

    SLog::log << Log::MESSAGE << "Processing file " << params.filename << Log::endl;
    SLog::log << Log::MESSAGE << "Subdivision level: " << params.subdivisions << Log::endl;

    std::shared_ptr<pic::Image> image = std::make_shared<pic::Image>(params.filename);


    std::vector<double> stdDevMultilevel = applyMultilevel([&](std::shared_ptr<pic::Image> img, int x, int y, int w, int h) {
        return getStdDev(image, x, y, w, h);
    }, image, params.subdivisions);

    SLog::log << Log::MESSAGE << "Standard deviations along all specified levels: " << Log::endl;
    for (auto& v : stdDevMultilevel)
        std::cout << v << " ";
    std::cout << std::endl;
}
