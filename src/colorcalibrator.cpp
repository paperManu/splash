#include "colorcalibrator.h"

#define PIC_DISABLE_OPENGL
#define PIC_DISABLE_QT

#include <piccante.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

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
    vector<float> maxValues(3, numeric_limits<float>::max());

    _nbrImageHDR = 3;

    while (true)
    {
        _crf.reset();

        shared_ptr<pic::Image> hdr;
        hdr = captureHDR();
        coords = getMaxRegionCenter(hdr);
        maxValues = getMeanValue(hdr);

        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum values: " << maxValues[0] << " - " << maxValues[1] << " - " << maxValues[2] << Log::endl;

        std::sort(maxValues.begin(), maxValues.end());
        if (maxValues[2] > 8.f)
        {
            Values res;
            _gcamera->getAttribute("shutterspeed", res);
            float speed = res[0].asFloat() * log2(maxValues[2]);
            _gcamera->setAttribute("shutterspeed", {speed});
        }
        else if (maxValues[2] < 1.0f)
        {
            Values res;
            _gcamera->getAttribute("shutterspeed", res);
            float speed = res[0].asFloat() / log2(maxValues[2]);
            _gcamera->setAttribute("shutterspeed", {speed});
        }
        else
        {
            _crf.reset();
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
    findCorrectExposure();
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {0});
        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});
    }

    // Compute the camera response function
    _nbrImageHDR = 5;
    _hdrStep = 0.5;
    captureHDR();
    _hdrStep = 1.0;
    _nbrImageHDR = 2;

    for (auto& cam : cameraList)
        world->sendMessage(cam.asString(), "hide", {1});

    shared_ptr<pic::Image> ambientHDR = captureHDR();

    // Get the Camera list
    vector<CalibrationParams> calibrationParams;

    // Find the location of each projection
    shared_ptr<pic::Image> hdr;
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "clearColor", {1.0, 1.0, 1.0, 1.0});
        hdr = captureHDR();
        *hdr -= *ambientHDR;
        vector<int> coords = getMaxRegionCenter(hdr);
        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});

        // Save the camera center for later use
        CalibrationParams params;
        params.camPos = coords;
        calibrationParams.push_back(params);
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

                Values color(4, 0.0);
                color[c] = x;
                color[3] = 1.0;

                world->sendMessage(camName, "clearColor", color);
                hdr = captureHDR();
                *hdr -= *ambientHDR;
                vector<float> values = getMeanValue(hdr, calibrationParams[i].camPos);
                world->sendMessage(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

                SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Camera " << camName << ", color channel " << c << " value: " << values[c] << " for input value: " << x << Log::endl;

                calibrationParams[i].curves[c].push_back(Point(x, values[c]));
            }

            // Update min and max values
            calibrationParams[i].minValues[c] = calibrationParams[i].curves[c][0].second;
            calibrationParams[i].maxValues[c] = calibrationParams[i].curves[c][samples - 1].second;
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
                v.second = v.second * scale + offset;
        }
    }

    // Send calibration to the cameras
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();
        Values lut;
        for (unsigned int v = 0; v < 256; ++v)
            for (unsigned int c = 0; c < 3; ++c)
                lut.push_back(calibrationParams[i].projectorCurves[c][v].second);

        world->sendMessage(camName, "colorLUT", {lut});
        world->sendMessage(camName, "activateColorLUT", {1});
    }

    // Cameras back to normal
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {0});
        world->sendMessage(cam.asString(), "flashBG", {0});
        world->sendMessage(cam.asString(), "clearColor", {});
    }

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Calibration updated" << Log::endl;
    
    _gcamera.reset();
    _crf.reset();
}

/*************/
vector<ColorCalibrator::Curve> ColorCalibrator::computeProjectorFunctionInverse(vector<Curve> rgbCurves)
{
    gsl_set_error_handler(gslErrorHandler);

    vector<Curve> projInvCurves;

    // Work on each curve independently
    for (auto& curve : rgbCurves)
    {
        double yOffset = curve[0].second;
        double yRange = curve[curve.size() - 1].second - curve[0].second;
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
            return a.second < b.second;
        });
        double previousAbscissa = -1.0;
        for (auto& point : curve)
        {
            double abscissa = (point.second - yOffset) / yRange; 
            if (abscissa == previousAbscissa)
            {
                SLog::log << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Abscissa not strictly increasing: discarding value" << Log::endl;
                continue;
            }
            previousAbscissa = abscissa;

            rawX.push_back((point.second - yOffset) / yRange);
            rawY.push_back(point.first);
        }

        // Check that first and last points abscissas are 0 and 1, respectively, and shift them slightly
        // to prevent floating point imprecision to cause an interpolation error
        rawX[0] = std::max(0.0, rawX[0]) - 0.001;
        rawX[rawX.size() - 1] = std::min(1.0, rawX[rawX.size() - 1]) + 0.001;

        gsl_interp_accel* acc = gsl_interp_accel_alloc();
        gsl_spline* spline = gsl_spline_alloc(gsl_interp_cspline, curve.size());
        gsl_spline_init(spline, rawX.data(), rawY.data(), curve.size());

        Curve projInvCurve;
        for (double x = 0.0; x <= 255.0; x += 1.0)
        {
            double realX = std::min(1.0, x / 255.0); // Make sure we don't try to go past 1.0
            Point point;
            point.first = realX;
            point.second = gsl_spline_eval(spline, realX, acc);
            projInvCurve.push_back(point);
        }
        projInvCurves.push_back(projInvCurve);

        gsl_spline_free(spline);
        gsl_interp_accel_free(acc);
    }

    gsl_set_error_handler_off();

    return projInvCurves;
}

/*************/
shared_ptr<pic::Image> ColorCalibrator::captureHDR()
{
    // Capture LDR images
    // Get the current shutterspeed
    Values res;
    _gcamera->getAttribute("shutterspeed", res);
    double defaultSpeed = res[0].asFloat();
    double nextSpeed = defaultSpeed;

    // Compute the parameters of the first capture
    for (int steps = _nbrImageHDR / 2; steps > 0; --steps)
        nextSpeed /= pow(2.0, _hdrStep);

    vector<pic::Image> ldr(_nbrImageHDR);
    vector<float> actualShutterSpeeds(_nbrImageHDR);
    for (unsigned int i = 0; i < _nbrImageHDR; ++i)
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
        nextSpeed *= pow(2.0, _hdrStep);
    }

    // Reset the shutterspeed
    _gcamera->setAttribute("shutterspeed", {defaultSpeed});

    // Check that all is well
    bool isValid = true;
    for (auto& image : ldr)
        isValid |= image.isValid();

    // Align all exposures on the well-exposed one
    int medianLDRIndex = _nbrImageHDR / 2 + 1;
    vector<Eigen::Vector2i> shift(_nbrImageHDR);
    for (unsigned int i = 0; i < _nbrImageHDR; ++i)
    {
        if (i == medianLDRIndex)
            continue;

        pic::Image* shiftedImage = pic::WardAlignment::Execute(&ldr[medianLDRIndex], &ldr[i], shift[i]);
        ldr[i].Assign(shiftedImage);
        delete shiftedImage;
    }

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

    for (unsigned int i = 0; i < _nbrImageHDR; ++i)
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

    // Region of interest contains all pixels at least half
    // as bright as the maximum
    int maxX {0}, maxY {0}, minX {image->width}, minY {image->height};
    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
        {
            float* pixel = (*image)(x, y);
            float linlum = 0.f;
            for (int c = 0; c < image->channels; ++c)
                linlum += pixel[c];
            if (linlum >= maxLinearLuminance * 0.5f)
            {
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        }

    coords = vector<int>({(maxX + minX) / 2, (maxY + minY) / 2});

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum found around point (" << coords[0] << ", " << coords[1] << ")" << Log::endl;

    return coords;
}

/*************/
vector<float> ColorCalibrator::getMeanValue(shared_ptr<pic::Image> image, vector<int> coords)
{
    vector<float> meanMaxValue(image->channels, numeric_limits<float>::min());
    
    if (coords.size() == 2)
    {
        pic::BBox box(coords[0] - _meanBoxSize / 2, coords[0] + _meanBoxSize / 2,
                      coords[1] - _meanBoxSize / 2, coords[1] + _meanBoxSize / 2);

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
