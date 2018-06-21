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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
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

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "cgUtils.h"
#include "log.h"

using namespace Splash;

/*************/
struct Parameters
{
    bool valid {true};
    std::string filename {""};
    unsigned int subdivisions = 0;
    bool silent {false};
    bool outputImages {false};

    cv::Mat3f image{};
    cv::Mat1b mask{};
};

/*************/
void showHelp()
{
    using std::cout;
    using std::endl;

    cout << "Splash calibration checker" << endl;
    cout << "A very simple tool to test projector calibration" << endl;
    cout << endl;
    cout << "Usage:" << endl;
    cout << " --help (-h): this very help" << endl;
    cout << " -f (--file) [filename]: specify the hdr image to test" << endl;
    cout << " -s (--subdiv) [subdivlevel]: specify the subdivision level for the test" << endl;
    cout << " -m (--mask) [filename]: set a mask from a tga B&W image" << endl;
    cout << " -b (--batch): only output the result with no info (useful for batch test)" << endl;
    cout << " -i (--image): output images named splash_check_[i].tga, i being the subdivision level" << endl;

    exit(0);
}

/*************/
Parameters parseArgs(int argc, char** argv)
{
    using std::string;

    Parameters params;

    // Get params
    if (argc == 1)
        showHelp();

    for (unsigned int i = 0; i < argc;)
    {
        if ((string(argv[i]) == "-f" || string(argv[i]) == "--file")&& i < argc - 1)
        {
            ++i;
            params.filename = string(argv[i]);
            params.image = cv::imread(params.filename);
        }
        else if ((string(argv[i]) == "-s" || string(argv[i]) == "--subdiv")&& i < argc - 1)
        {
            ++i;
            params.subdivisions = std::stoi(string(argv[i]));
        }
        else if ((string(argv[i]) == "-m" || string(argv[i]) == "--mask")&& i < argc - 1)
        {
            ++i;
            string filename = string(argv[i]);
            if (filename.find("tga") == string::npos)
            {
                params.valid = false;
                Log::get() << Log::WARNING << "Please specify a TGA (non-RLE encoded) file for the mask." << Log::endl;
            }
            else
            {
                params.mask = cv::imread(filename, cv::IMREAD_GRAYSCALE);
            }
        }
        else if (string(argv[i]) == "-b" || string(argv[i]) == "--batch")
        {
            params.silent = true;
            Log::get().setVerbosity(Log::NONE);
        }
        else if (string(argv[i]) == "-i" || string(argv[i]) == "--image")
        {
            params.outputImages = true;
        }
        else if (string(argv[i]) == "-h" || string(argv[i]) == "--help")
        {
            showHelp();
        }
        ++i;
    }
    
    // Check params
    if (params.filename.find("hdr") == string::npos)
    {
        params.valid = false;
        Log::get() << Log::WARNING << "Please specify a HDR file to process." << Log::endl;
    }
    if (params.image.total() == 0)
    {
        params.valid = false;
        Log::get() << Log::WARNING << "Could not open file " << params.filename << ". Exiting." << Log::endl;
    }
    if (params.mask.total() != 0)
    {
        if (params.mask.cols != params.image.cols || params.mask.rows != params.image.rows)
        {
            params.valid = false;
            Log::get() << Log::WARNING << "Could not open file " << params.filename << ". Exiting." << Log::endl;
        }
    }

    return params;
}

/*************/
double getStdDev(Parameters& params, int x = 0, int y = 0, int w = 0, int h = 0)
{
    using std::cout;
    using std::endl;

    if (params.image.total() != 0)
        return 0.0;

    bool withMask = false;
    if (params.mask.total() != 0)
        withMask = true;

    int width = w == 0 ? params.image.cols : std::min(w, params.image.cols);
    int height = h == 0 ? params.image.rows : std::min(h, params.image.rows);

    pic::BBox roi = pic::BBox(x, x+width, y, y+height);
    double totalPixels = 0.0;

    double maxValue = 0.0;
    double meanValue = 0.0;
    for (int y = roi.y0; y < roi.y1; ++y)
        for (int x = roi.x0; x < roi.x1; ++x)
        {
            if (withMask)
            {
                unsigned char* maskPixel = params.mask.ptr();
                if (maskPixel[(y * params.mask.cols + x) * params.mask.channels] < 128)
                {
                    continue;
                }
            }
            totalPixels++;

            float* pixel = params.image(y, x);
            RgbValue rgb(pixel[0], pixel[1], pixel[2]);
            meanValue += rgb.luminance();

            maxValue = std::max(maxValue, (double)rgb.luminance());
        }
    meanValue = totalPixels == 0.0 ? 0.0 : meanValue / totalPixels;

    double stdDev = 0.0;
    for (int y = roi.y0; y < roi.y1; ++y)
        for (int x = roi.x0; x < roi.x1; ++x)
        {
            if (withMask)
            {
                unsigned char* maskPixel = params.mask.ptr();
                if (maskPixel[(y * params.mask.cols + x) * params.mask.channels] < 128)
                    continue;
            }

            float* pixel = image(y, x);
            RgbValue rgb(pixel[0], pixel[1], pixel[2]);
            double lum = rgb.luminance();
            stdDev += std::pow(lum - meanValue, 2.0);
        }

    stdDev = totalPixels == 0.0 ? 0.0 : std::sqrt(stdDev / totalPixels);

    return stdDev;
}

/*************/
std::vector<std::vector<double>> applyMultilevel(std::function<double(Parameters&, int, int, int, int)> filter, Parameters& params)
{
    std::vector<std::vector<double>> results;

    for (unsigned int l = 0; l <= params.subdivisions; ++l)
    {
        int blockWidth = params.image.cols / std::pow(2.0, l);
        int blockHeight = params.image.rows / std::pow(2.0, l);

        std::vector<double> subdivResults;
        for (int y = 0; y < params.image.rows; y += blockHeight)
            for (int x = 0; x < params.image.cols; x += blockWidth)
            {
                double result = filter(params, x, y, blockWidth, blockHeight);
                subdivResults.push_back(result);
            }

        results.push_back(subdivResults);
    }

    return results;
}

/*************/
#define OUTPUT_SIZE 512
void saveImagesFromMultilevel(Parameters params, std::vector<std::vector<double>> results)
{
    int index = 0;

    double maxStdDev = 0.0;
    for (auto& result : results)
        for (auto& v : result)
            maxStdDev = std::max(maxStdDev, v);

    for (auto& result : results)
    {
        std::vector<unsigned char> image(OUTPUT_SIZE * OUTPUT_SIZE);
        int subdiv = round(sqrt(result.size()));
        int step = OUTPUT_SIZE / subdiv;

        for (int y = 0; y < OUTPUT_SIZE; ++y)
        {
            for (int x = 0; x < OUTPUT_SIZE; ++x)
            {
                int col = x / step;
                int row = y / step;

                image[y * OUTPUT_SIZE + x] = (unsigned char)(result[row * subdiv + col] / maxStdDev * 255.0);
            }
        }

        pic::WriteTGA("/tmp/splash_check_" + std::to_string(index) + ".tga", image.data(), OUTPUT_SIZE, OUTPUT_SIZE, 1);
        index++;
    }
}

/*************/
int main(int argc, char** argv)
{
    Parameters params = parseArgs(argc, argv);

    if (!params.valid)
    {
        Log::get() << Log::WARNING << "An error was found in the parameters, exiting." << Log::endl;
        exit(1);
    }

    Log::get() << Log::MESSAGE << "Processing file " << params.filename << Log::endl;
    Log::get() << Log::MESSAGE << "Subdivision level: " << params.subdivisions << Log::endl;

    std::vector<std::vector<double>> stdDevMultilevel = applyMultilevel([&](Parameters& params, int x, int y, int w, int h) {
        return getStdDev(params, x, y, w, h);
    }, params);

    Log::get() << Log::MESSAGE << "Standard deviations along all specified levels: " << Log::endl;
    for (auto& subdivResult : stdDevMultilevel)
    {
        for (auto& result : subdivResult)
            std::cout << result << " ";
        std::cout << std::endl;
    }

    if (params.outputImages)
        saveImagesFromMultilevel(params, stdDevMultilevel);
}
