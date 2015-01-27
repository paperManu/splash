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
void ColorCalibrator::findCorrectExposure()
{
    vector<int> coords(3, 0);
    vector<float> meanValues(3, numeric_limits<float>::max());

    while (true)
    {
        shared_ptr<pic::Image> hdr;
        hdr = captureHDR(3, 1.0);
        vector<int> coords = getMaxRegionCenter(hdr);
        meanValues = getMeanValue(hdr, coords, 256);

        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum values: " << meanValues[0] << " - " << meanValues[1] << " - " << meanValues[2] << Log::endl;

        std::sort(meanValues.begin(), meanValues.end());
        double lum = sqrt(meanValues[0]*meanValues[0] + meanValues[1]*meanValues[1] + meanValues[2]*meanValues[2]);
        if (lum > 8.f || lum < 1.0f)
        {
            Values res;
            _gcamera->getAttribute("shutterspeed", res);
            float speed = res[0].asFloat() * pow(2.0, log2(lum));
            _gcamera->setAttribute("shutterspeed", {speed});
        }
        else
        {
            break;
        }
    }
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

    // Find the correct central exposure time
    // All cameras to grey
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {1});
        world->sendMessage(cam.asString(), "flashBG", {1});
        world->sendMessage(cam.asString(), "clearColor", {0.5, 0.5, 0.5, 1.0});
    }
    //findCorrectExposure();
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {0});
        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});
    }

    // Compute the camera response function
    if (_crf == nullptr)
        captureHDR(7, 0.5);

    for (auto& cam : cameraList)
        world->sendMessage(cam.asString(), "hide", {1});

    // Get the Camera list
    vector<CalibrationParams> calibrationParams;

    // Find the location of each projection
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
        calibrationParams.push_back(params);
    }

    // Find color mixing matrix
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();

        vector<vector<float>> lowValues(3);
        vector<vector<float>> highValues(3);
        glm::mat3 mixRGB;
        for (int c = 0; c < 3; ++c)
        {
            // We take two HDR: at middle and max value for each channel
            // Then we compute the linear relation between this channel and
            // the other two
            Values color(4, 0.0);
            color[c] = 0.5;
            color[3] = 1.0;
            world->sendMessage(camName, "clearColor", color);
            hdr = captureHDR(1);
            lowValues[c] = getMeanValue(hdr, calibrationParams[i].camPos);

            color[c] = 1.0;
            world->sendMessage(camName, "clearColor", color);
            hdr = captureHDR(1);
            highValues[c] = getMeanValue(hdr, calibrationParams[i].camPos);
        }

        world->sendMessage(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

        for (int c = 0; c < 3; ++c)
            for (int otherC = 0; otherC < 3; ++otherC)
                mixRGB[otherC][c] = (highValues[c][otherC] - lowValues[c][otherC]) / (highValues[otherC][otherC] - lowValues[otherC][otherC]);

        calibrationParams[i].mixRGB = glm::inverse(mixRGB);
    }

    // Find color curves for each Camera
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();

        for (int c = 0; c < 3; ++c)
        {
            int samples = _colorCurveSamples;
            for (int s = 0; s <= samples; ++s)
            {
                float x = (float)s / (float)samples;

                // We apply the inverse rgb mixing matrix to separate as much as possible the channels
                glm::vec3 rgb;
                rgb[c] = x;
                rgb = glm::clamp(calibrationParams[i].mixRGB * rgb, glm::vec3(0.0), glm::vec3(1.0));

                Values color(4, 0.0);
                color[0] = rgb[0];
                color[1] = rgb[1];
                color[2] = rgb[2];
                color[3] = 1.0;

                world->sendMessage(camName, "clearColor", color);
                hdr = captureHDR(2, 1.0);
                vector<float> values = getMeanValue(hdr, calibrationParams[i].camPos);
                world->sendMessage(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

                SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Camera " << camName << ", color channel " << c << " value: " << values[c] << " for input value: " << x << Log::endl;

                calibrationParams[i].curves[c].push_back(Point(x, values));
            }

            // Update min and max values
            calibrationParams[i].minValues[c] = calibrationParams[i].curves[c][0].second[c];
            calibrationParams[i].maxValues[c] = calibrationParams[i].curves[c][samples - 1].second[c];
        }

        calibrationParams[i].projectorCurves = computeProjectorFunctionInverse(calibrationParams[i].curves);
    }

    // Get the overall maximum value for rgb(0,0,0), and minimum for rgb(1,1,1)
    vector<float> minValues(3, 0.f);
    vector<float> maxValues(3, numeric_limits<float>::max());
    for (auto& params : calibrationParams)
    {
        for (unsigned int c = 0; c < 3; ++c)
        {
            minValues[c] = std::max(minValues[c], params.minValues[c]);
            maxValues[c] = std::min(maxValues[c], params.maxValues[c]);
        }
    }

    // Offset and scale projector curves to fit in [minValue, maxValue] for all channels
    for (auto& params : calibrationParams)
    {
        for (unsigned int c = 0; c < 3; ++c)
        {
            double range = params.maxValues[c] - params.minValues[c];
            double offset = (minValues[c] - params.minValues[c]) / range;
            double scale = (maxValues[c] - minValues[c]) / (params.maxValues[c] - params.minValues[c]);

            for (auto& v : params.projectorCurves[c])
                v.second.set(c, v.second[c] * scale + offset);
        }
    }

    // Send calibration to the cameras
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

    // Cameras back to normal
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

    _crf.reset();
    //findCorrectExposure();
    // Compute the camera response function
    _crf.reset();
    captureHDR(7, 0.5);

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
        _crf->DebevecMalik(stack, actualShutterSpeeds.data());
    }

    for (unsigned int i = 0; i < nbrLDR; ++i)
        stack[i]->exposure = actualShutterSpeeds[i];

    // Assemble the images into a single HDRI
    pic::FilterAssembleHDR assembleHDR(pic::CRF_GAUSS, pic::LIN_ICFR, &_crf->icrf);
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
        double previousAbscissa = -1.0;
        for (auto& point : curve)
        {
            double abscissa = (point.second[c] - yOffset) / yRange; 
            if (abscissa == previousAbscissa)
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
        if (rawX.size() >= 4)
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
