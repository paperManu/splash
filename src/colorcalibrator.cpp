#include "colorcalibrator.h"

#define PIC_DISABLE_OPENGL
#define PIC_DISABLE_QT

#include <piccante.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#define GLM_FORCE_SSE2
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/simd_mat4.hpp>
#include <glm/gtx/simd_vec4.hpp>

#include "image_gphoto.h"
#include "log.h"
#include "timer.h"
#include "threadpool.h"
#include "world.h"

using namespace std;

namespace Splash
{

/*************/
void gslErrorHandler(const char* reason, const char* file, int line, int gsl_errno)
{
    string errorString = string(reason);
    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - An error in a GSL function has be caught: " << errorString << Log::endl;
}

/*************/
ColorCalibrator::ColorCalibrator(std::weak_ptr<World> world)
{
    _world = world;
    registerAttributes();
}

/*************/
ColorCalibrator::~ColorCalibrator()
{
}

/*************/
float rgbToLuminance(float r, float g, float b)
{
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

/*************/
float rgbToLuminance(vector<float> rgb)
{
    if (rgb.size() < 3)
        return 0.f;
    return rgbToLuminance(rgb[0], rgb[1], rgb[2]);
}

/*************/
void ColorCalibrator::update()
{
    // Initialize camera
    _gcamera = make_shared<Image_GPhoto>();

    // Check whether the camera is ready
    Values status;
    _gcamera->getAttribute("ready", status);
    if (status.size() == 0 || status[0].asInt() == 0)
    {
        SLog::log << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Camera is not ready, unable to update calibration" << Log::endl;
        return;
    }

    auto world = _world.lock();
    Values cameraList = world->getObjectsNameByType("camera");

    //
    // Find the exposure times for all black and all white
    //
    float mediumExposureTime;
    // All cameras to white
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {1});
        world->sendMessage(cam.asString(), "flashBG", {1});
        world->sendMessage(cam.asString(), "clearColor", {0.7, 0.7, 0.7, 1.0});
    }
    mediumExposureTime = findCorrectExposure();

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Exposure time: " << mediumExposureTime << Log::endl;

    for (auto& cam : cameraList)
        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});

    // All cameras to normal
    for (auto& cam : cameraList)
        world->sendMessage(cam.asString(), "hide", {0});

    //
    // Compute the camera response function
    //
    if (_crf == nullptr)
        captureHDR(9, 0.33);

    for (auto& cam : cameraList)
        world->sendMessage(cam.asString(), "hide", {1});

    // Get the Camera list
    vector<CalibrationParams> calibrationParams;

    //
    // Find the location of each projection
    //
    _gcamera->setAttribute("shutterspeed", {mediumExposureTime});
    shared_ptr<pic::Image> hdr;
    for (auto& cam : cameraList)
    {
        // Activate the target projector
        world->sendMessage(cam.asString(), "clearColor", {1.0, 1.0, 1.0, 1.0});
        hdr = captureHDR(1);

        // Activate all the other ones
        for (auto& otherCam : cameraList)
            world->sendMessage(otherCam.asString(), "clearColor", {1.0, 1.0, 1.0, 1.0});
        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});
        shared_ptr<pic::Image> othersHdr = captureHDR(1);

        *hdr -= *othersHdr;
        vector<int> coords = getMaxRegionCenter(hdr);
        for (auto& otherCam : cameraList)
            world->sendMessage(otherCam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});

        // Save the camera center for later use
        CalibrationParams params;
        params.camPos = coords;
        params.whitePoint = getMeanValue(hdr, coords);
        calibrationParams.push_back(params);
    }

    //
    // Find color curves for each Camera
    //
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();

        for (int c = 0; c < 3; ++c)
        {
            int samples = _colorCurveSamples;
            for (int s = 0; s <= samples; ++s)
            {
                float x = (float)s / (float)samples;

                // Set the color
                Values color(4, 0.0);
                color[c] = x;
                color[3] = 1.0;
                world->sendMessage(camName, "clearColor", color);

                // Set approximately the exposure
                _gcamera->setAttribute("shutterspeed", {mediumExposureTime});

                hdr = captureHDR(2, 1.0);
                vector<float> values = getMeanValue(hdr, calibrationParams[i].camPos, 64);
                calibrationParams[i].curves[c].push_back(Point(x, values));

                world->sendMessage(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});
                SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Camera " << camName << ", color channel " << c << " value: " << values[c] << " for input value: " << x << Log::endl;
            }

            // Update min and max values, added to the black level
            calibrationParams[i].minValues.set(c, calibrationParams[i].curves[c][0].second[c]);
            calibrationParams[i].maxValues.set(c, calibrationParams[i].curves[c][samples - 1].second[c]);
        }

        calibrationParams[i].projectorCurves = computeProjectorFunctionInverse(calibrationParams[i].curves);
    }

    //
    // Find color mixing matrix
    //
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();

        vector<vector<float>> lowValues(3);
        vector<vector<float>> highValues(3);
        glm::mat3 mixRGB;

        // Get the middle and max values from the previous captures
        for (int c = 0; c < 3; ++c)
        {
            lowValues[c].push_back(calibrationParams[i].curves[c][1].second[0]);
            lowValues[c].push_back(calibrationParams[i].curves[c][1].second[1]);
            lowValues[c].push_back(calibrationParams[i].curves[c][1].second[2]);

            highValues[c].push_back(calibrationParams[i].curves[c][_colorCurveSamples - 1].second[0]);
            highValues[c].push_back(calibrationParams[i].curves[c][_colorCurveSamples - 1].second[1]);
            highValues[c].push_back(calibrationParams[i].curves[c][_colorCurveSamples - 1].second[2]);
        }

        world->sendMessage(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

        for (int c = 0; c < 3; ++c)
            for (int otherC = 0; otherC < 3; ++otherC)
                mixRGB[otherC][c] = (highValues[c][otherC] - lowValues[c][otherC]) / (highValues[otherC][otherC] - lowValues[otherC][otherC]);

        calibrationParams[i].mixRGB = glm::inverse(mixRGB);
    }

    //
    // Compute and apply the white balance
    //
    RgbValue meanWhiteBalance;
    for (auto& params : calibrationParams)
    {
        params.whiteBalance = params.whitePoint / params.whitePoint[1];
        meanWhiteBalance = meanWhiteBalance + params.whiteBalance;
    }

    meanWhiteBalance = meanWhiteBalance / (float)calibrationParams.size();

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Mean white balance: " << meanWhiteBalance[0] << " / " << meanWhiteBalance[1] << " / " << meanWhiteBalance[2] << Log::endl;

    int index = 0;
    for (auto& params : calibrationParams)
    {
        RgbValue whiteBalance;
        whiteBalance = meanWhiteBalance / params.whiteBalance;
        whiteBalance.normalize();
        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " Projector " << index << " white balance: " << whiteBalance[0] << " / " << whiteBalance[1] << " / " << whiteBalance[2] << Log::endl;

        for (unsigned int c = 0; c < 3; ++c)
            for (auto& v : params.projectorCurves[c])
                v.second.set(c, v.second[c] * whiteBalance[c]);

        params.minValues = params.minValues * whiteBalance;
        params.maxValues = params.maxValues * whiteBalance;

        index++;
    }

    //
    // Get the overall maximum value for rgb(0,0,0), and minimum for rgb(1,1,1)
    //
    // Fourth values contain luminance (calculated using other values)
    vector<float> minValues(4, 0.f);
    vector<float> maxValues(4, numeric_limits<float>::max());
    for (auto& params : calibrationParams)
    {
        for (unsigned int c = 0; c < 3; ++c)
        {
            minValues[c] = std::max(minValues[c], params.minValues[c]);
            maxValues[c] = std::min(maxValues[c], params.maxValues[c]);
        }
        minValues[3] = std::max(minValues[3], params.minValues.luminance());
        maxValues[3] = std::min(maxValues[3], params.maxValues.luminance());
    }

    //
    // Offset and scale projector curves to fit in [minValue, maxValue] for all channels
    //
    for (auto& params : calibrationParams)
    {
        // We use a common scale and offset to keep color balance
        float range = params.maxValues.luminance() - params.minValues.luminance();
        float offset = (minValues[3] - params.minValues.luminance()) / range;
        float scale = (maxValues[3] - minValues[3]) / range;

        for (unsigned int c = 0; c < 3; ++c)
            for (auto& v : params.projectorCurves[c])
                v.second.set(c, v.second[c] * scale + offset);
    }

    //
    // Send calibration to the cameras
    //
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();
        Values lut;
        for (unsigned int v = 0; v < 256; ++v)
            for (unsigned int c = 0; c < 3; ++c)
                lut.push_back(calibrationParams[i].projectorCurves[c][v].second[c]);

        world->sendMessage(camName, "colorLUT", {lut});
        world->sendMessage(camName, "activateColorLUT", {1});

        Values m(9);
        for (int u = 0; u < 3; ++u)
            for (int v = 0; v < 3; ++v)
                m[u*3 + v] = calibrationParams[i].mixRGB[u][v];

        world->sendMessage(camName, "colorMixMatrix", {m});
    }

    //
    // Cameras back to normal
    //
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {0});
        world->sendMessage(cam.asString(), "flashBG", {0});
        world->sendMessage(cam.asString(), "clearColor", {});
    }

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Calibration updated" << Log::endl;
    
    // Free camera
    _gcamera.reset();
}

/*************/
void ColorCalibrator::updateCRF()
{
    // Initialize camera
    _gcamera = make_shared<Image_GPhoto>();

    // Check whether the camera is ready
    Values status;
    _gcamera->getAttribute("ready", status);
    if (status.size() == 0 || status[0].asInt() == 0)
    {
        SLog::log << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Camera is not ready, unable to update color response" << Log::endl;
        return;
    }

    findCorrectExposure();

    // Compute the camera response function
    _crf.reset();
    captureHDR(9, 0.33);

    // Free camera
    _gcamera.reset();
}

/*************/
shared_ptr<pic::Image> ColorCalibrator::captureHDR(unsigned int nbrLDR, double step)
{
    // Capture LDR images
    // Get the current shutterspeed
    Values res;
    _gcamera->getAttribute("shutterspeed", res);
    double defaultSpeed = res[0].asFloat();
    double nextSpeed = defaultSpeed;

    // Compute the parameters of the first capture
    for (int steps = nbrLDR / 2; steps > 0; --steps)
        nextSpeed /= pow(2.0, step);

    vector<pic::Image> ldr(nbrLDR);
    vector<float> actualShutterSpeeds(nbrLDR);
    for (unsigned int i = 0; i < nbrLDR; ++i)
    {
        _gcamera->setAttribute("shutterspeed", {nextSpeed});
        // We get the actual shutterspeed
        _gcamera->getAttribute("shutterspeed", res);
        nextSpeed = res[0].asFloat();

        actualShutterSpeeds[i] = nextSpeed;

        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Capturing LDRI with a " << nextSpeed << "sec exposure time" << Log::endl;

        string filename = "/tmp/splash_ldr_sample_" + to_string(i) + ".tga";
        _gcamera->capture();
        _gcamera->update();
        _gcamera->write(filename);

        ldr[i].Read(filename, pic::LT_NOR);

        // Update exposure for next step
        nextSpeed *= pow(2.0, step);
    }

    // Reset the shutterspeed
    _gcamera->setAttribute("shutterspeed", {defaultSpeed});

    // Check that all is well
    bool isValid = true;
    for (auto& image : ldr)
        isValid |= image.isValid();

    vector<pic::Image*> stack;
    for (auto& image : ldr)
        stack.push_back(&image);

    // Estimate camera response function, if needed
    if (_crf == nullptr)
    {
        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Generating camera response function" << Log::endl;
        _crf = make_shared<pic::CameraResponseFunction>();
        _crf->DebevecMalik(stack, actualShutterSpeeds.data(), pic::CRF_AKYUZ, 200);
    }

    for (unsigned int i = 0; i < nbrLDR; ++i)
        stack[i]->exposure = actualShutterSpeeds[i];

    // Assemble the images into a single HDRI
    pic::FilterAssembleHDR assembleHDR(pic::CRF_AKYUZ, pic::LIN_ICFR, &_crf->icrf);
    pic::Image* temporaryHDR = assembleHDR.ProcessP(stack, nullptr);

    shared_ptr<pic::Image> hdr = make_shared<pic::Image>();
    hdr->Assign(temporaryHDR);
    delete temporaryHDR;

    hdr->Write("/tmp/splash_hdr.hdr");
    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - HDRI computed" << Log::endl;

    return hdr;
}

/*************/
vector<ColorCalibrator::Curve> ColorCalibrator::computeProjectorFunctionInverse(vector<Curve> rgbCurves)
{
    gsl_set_error_handler(gslErrorHandler);

    vector<Curve> projInvCurves;

    // Work on each curve independently
    unsigned int c = 0; // index of the current channel
    for (auto& curve : rgbCurves)
    {
        double yOffset = curve[0].second[c];
        double yRange = curve[curve.size() - 1].second[c] - curve[0].second[c];
        if (yRange <= 0.f)
        {
            SLog::log << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Unable to compute projector inverse function curve on a channel" << Log::endl;
            projInvCurves.push_back(Curve());
            continue;
        }

        vector<double> rawX;
        vector<double> rawY;

        // Make sure the points are correctly ordered
        sort(curve.begin(), curve.end(), [&](Point a, Point b) {
            return a.second[c] < b.second[c];
        });

        double epsilon = 0.001;
        double previousAbscissa = -1.0;
        for (auto& point : curve)
        {
            double abscissa = (point.second[c] - yOffset) / yRange; 
            if (abs(abscissa - previousAbscissa) < epsilon)
            {
                SLog::log << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Abscissa not strictly increasing: discarding value" << Log::endl;
                continue;
            }
            previousAbscissa = abscissa;

            rawX.push_back((point.second[c] - yOffset) / yRange);
            rawY.push_back(point.first);
        }

        // Check that first and last points abscissas are 0 and 1, respectively, and shift them slightly
        // to prevent floating point imprecision to cause an interpolation error
        rawX[0] = std::max(0.0, rawX[0]) - 0.001;
        rawX[rawX.size() - 1] = std::min(1.0, rawX[rawX.size() - 1]) + 0.001;

        gsl_interp_accel* acc = gsl_interp_accel_alloc();
        gsl_spline* spline = nullptr;
        if (rawX.size() > 4)
            spline = gsl_spline_alloc(gsl_interp_akima, curve.size());
        else
            spline = gsl_spline_alloc(gsl_interp_linear, curve.size());
        gsl_spline_init(spline, rawX.data(), rawY.data(), curve.size());

        Curve projInvCurve;
        for (double x = 0.0; x <= 255.0; x += 1.0)
        {
            double realX = std::min(1.0, x / 255.0); // Make sure we don't try to go past 1.0
            Point point;
            point.first = realX;
            point.second.set(c, gsl_spline_eval(spline, realX, acc));
            projInvCurve.push_back(point);
        }
        projInvCurves.push_back(projInvCurve);

        gsl_spline_free(spline);
        gsl_interp_accel_free(acc);

        c++; // increment the current channel
    }

    gsl_set_error_handler_off();

    return projInvCurves;
}

/*************/
double computeMoment(shared_ptr<pic::Image> image, int i, int j, double targetLum = 0.0)
{
    double moment = 0.0;

    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
        {
            float* pixel = (*image)(x, y);
            double linlum = 0.f;
            for (int c = 0; c < image->channels; ++c)
                linlum += pixel[c];
            if (targetLum == 0.0)
                moment += pow(x, i) * pow(y, j) * linlum;
            else if (linlum >= targetLum * 0.5)
                moment += pow(x, i) * pow(y, j);
        }

    return moment;
}

/*************/
float ColorCalibrator::findCorrectExposure()
{
    Values res;
    while (true)
    {
        _gcamera->getAttribute("shutterspeed", res);
        _gcamera->capture();
        _gcamera->update();
        oiio::ImageBuf img = _gcamera->get();
        oiio::ImageSpec spec = _gcamera->getSpec();

        unsigned long total = spec.width * spec.height;
        unsigned long sum = 0;
        for (oiio::ImageBuf::ConstIterator<unsigned char> p(img); !p.done(); ++p)
        {
            if (!p.exists())
                continue;

            sum += (unsigned long)(255.f * (0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]));
        }

        float meanValue = (float)sum / (float)total;
        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Mean value over all channels: " << meanValue << Log::endl;

        if (meanValue < 100.f)
        {
            float speed = res[0].asFloat() * std::max(1.5f, 100.f / meanValue);
            _gcamera->setAttribute("shutterspeed", {speed});
        }
        else if (meanValue > 160.f)
        {
            float speed = res[0].asFloat() / std::max(1.5f, 160.f / meanValue);
            _gcamera->setAttribute("shutterspeed", {speed});
        }
        else
        {
            break;
        }
    }
    if (res.size() == 0)
        return 0.f;
    else
        return res[0].asFloat();
}

/*************/
vector<int> ColorCalibrator::getMaxRegionCenter(shared_ptr<pic::Image> image)
{
    if (image == nullptr || !image->isValid())
        return vector<int>();

    vector<int> coords;

    // Find the maximum value
    float maxLinearLuminance = numeric_limits<float>::min();
    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
        {
            float* pixel = (*image)(x, y);
            float linlum = 0.f;
            for (int c = 0; c < image->channels; ++c)
                linlum += pixel[c];
            if (linlum > maxLinearLuminance)
                maxLinearLuminance = linlum;
        }

    // Compute the binary moments of all pixels brighter than maxLinearLuminance
    vector<double> moments(3, 0.0);
    moments[0] = computeMoment(image, 0, 0, maxLinearLuminance);
    moments[1] = computeMoment(image, 1, 0, maxLinearLuminance);
    moments[2] = computeMoment(image, 0, 1, maxLinearLuminance);

    coords = vector<int>({(int)(moments[1] / moments[0]), (int)(moments[2] / moments[0])});

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum found around point (" << coords[0] << ", " << coords[1] << ")" << Log::endl;

    return coords;
}

/*************/
vector<float> ColorCalibrator::getMeanValue(shared_ptr<pic::Image> image, vector<int> coords, int boxSize)
{
    vector<float> meanMaxValue(image->channels, numeric_limits<float>::min());
    
    if (coords.size() == 2)
    {
        pic::BBox box(coords[0] - boxSize / 2, coords[0] + boxSize / 2,
                      coords[1] - boxSize / 2, coords[1] + boxSize / 2);

        image->getMeanVal(&box, meanMaxValue.data());
    }
    else
    {
        image->getMeanVal(nullptr, meanMaxValue.data());
    }

    return meanMaxValue;
}

/*************/
void ColorCalibrator::registerAttributes()
{
}

} // end of namespace
