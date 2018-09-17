#include "./controller/colorcalibrator.h"

#define PIC_DISABLE_OPENGL
#define PIC_DISABLE_QT

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#define GLM_FORCE_SSE2
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <opencv2/highgui.hpp>

#include "./image/image_gphoto.h"
#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
void gslErrorHandler(const char* reason, const char* file, int line, int gsl_errno)
{
    string errorString = string(reason);
    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - An error in a GSL function has be caught: " << errorString << Log::endl;
}

/*************/
ColorCalibrator::ColorCalibrator(RootObject* root)
    : ControllerObject(root)
{
    _type = "colorCalibrator";
    registerAttributes();

    // Set up the calibration strategy
    equalizeWhiteBalances = std::bind(&ColorCalibrator::equalizeWhiteBalancesMaximizeMinLum, this);
}

/*************/
void ColorCalibrator::update()
{
    if (!_calibrationMutex.try_lock())
    {
        Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Another calibration is in process. Exiting." << Log::endl;
        return;
    }

    if (_calibrationThread.valid())
        _calibrationThread.wait();

    _calibrationThread = async(launch::async, [&]() {
        // Initialize camera
        _gcamera = make_shared<Image_GPhoto>(_root, "");
        // Prepare for freeing the camera when leaving scope
        OnScopeExit
        {
            _calibrationMutex.unlock();
            _gcamera.reset();
        };

        // Check whether the camera is ready
        Values status;
        _gcamera->getAttribute("ready", status);
        if (status.size() == 0 || status[0].as<int>() == 0)
        {
            Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Camera is not ready, unable to update calibration" << Log::endl;
            return;
        }

        // Get the Camera list
        auto cameras = getObjectsPtr(getObjectsOfType("camera"));

        _calibrationParams.clear();
        for (const auto& camera : cameras)
        {
            CalibrationParams params;
            params.camName = camera->getName();
            _calibrationParams.push_back(params);
        }

        //
        // Find the exposure times for all black and all white
        //
        float mediumExposureTime;
        // All cameras to white
        for (auto& params : _calibrationParams)
        {
            setObjectAttribute(params.camName, "hide", {1});
            setObjectAttribute(params.camName, "flashBG", {1});
            setObjectAttribute(params.camName, "clearColor", {0.7, 0.7, 0.7, 1.0});
        }
        mediumExposureTime = findCorrectExposure();

        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Exposure time: " << mediumExposureTime << Log::endl;

        for (auto& params : _calibrationParams)
            setObjectAttribute(params.camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

        // All cameras to normal
        for (auto& params : _calibrationParams)
            setObjectAttribute(params.camName, "hide", {0});

        //
        // Compute the camera response function
        //
        if (_crf.total() == 0)
            captureHDR(9, 0.33);

        for (auto& params : _calibrationParams)
            setObjectAttribute(params.camName, "hide", {1});

        //
        // Find the location of each projection
        //
        _gcamera->setAttribute("shutterspeed", {mediumExposureTime});
        cv::Mat hdr;
        for (auto& params : _calibrationParams)
        {
            // Activate the target projector
            setObjectAttribute(params.camName, "clearColor", {1.0, 1.0, 1.0, 1.0});
            hdr = captureHDR(1);
            if (hdr.total() == 0)
                return;

            // Activate all the other ones
            for (auto& otherCam : cameras)
                setObjectAttribute(otherCam->getName(), "clearColor", {1.0, 1.0, 1.0, 1.0});
            setObjectAttribute(params.camName, "clearColor", {0.0, 0.0, 0.0, 1.0});
            cv::Mat othersHdr = captureHDR(1);
            if (othersHdr.total() == 0)
                return;

            if (hdr.rows != othersHdr.rows || hdr.cols != othersHdr.cols)
                return;

            cv::Mat diffHdr = hdr.clone();
            for (uint32_t y = 0; y < diffHdr.rows; ++y)
                for (uint32_t x = 0; x < diffHdr.cols; ++x)
                {
                    auto pixelValue = diffHdr.at<cv::Vec3f>(y, x) - othersHdr.at<cv::Vec3f>(y, x) * _displayDetectionThreshold;
                    diffHdr.at<cv::Vec3f>(y, x)[0] = max(0.f, pixelValue[0]);
                    diffHdr.at<cv::Vec3f>(y, x)[1] = max(0.f, pixelValue[1]);
                    diffHdr.at<cv::Vec3f>(y, x)[2] = max(0.f, pixelValue[2]);
                }

            params.maskROI = getMaskROI(diffHdr);
            for (auto& otherCam : cameras)
                setObjectAttribute(otherCam->getName(), "clearColor", {0.0, 0.0, 0.0, 1.0});

            // Save the camera center for later use
            params.whitePoint = getMeanValue(hdr, params.maskROI);
        }

        //
        // Find color curves for each Camera
        //
        for (auto& params : _calibrationParams)
        {
            string camName = params.camName;

            RgbValue minValues;
            RgbValue maxValues;
            for (int c = 0; c < 3; ++c)
            {
                int samples = _colorCurveSamples;
                for (int s = 0; s < samples; ++s)
                {
                    auto x = static_cast<float>(s) / static_cast<float>(samples - 1);

                    // Set the color
                    Values color(4, 0.0);
                    color[c] = x;
                    color[3] = 1.0;
                    setObjectAttribute(camName, "clearColor", {color[0], color[1], color[2], color[3]});

                    // Set approximately the exposure
                    _gcamera->setAttribute("shutterspeed", {mediumExposureTime});

                    hdr = captureHDR(_imagePerHDR, _hdrStep);
                    if (hdr.total() == 0)
                        return;
                    vector<float> values = getMeanValue(hdr, params.maskROI);
                    params.curves[c].push_back(Point(x, values));

                    setObjectAttribute(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});
                    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Camera " << camName << ", color channel " << c << " value: " << values[c]
                               << " for input value: " << x << Log::endl;
                }

                // Update min and max values, added to the black level
                minValues[c] = params.curves[c][0].second[c];
                maxValues[c] = params.curves[c][samples - 1].second[c];
            }

            params.minValues = minValues;
            params.maxValues = maxValues;

            params.projectorCurves = computeProjectorFunctionInverse(params.curves);
        }

        //
        // Find color mixing matrix
        //
        for (auto& params : _calibrationParams)
        {
            string camName = params.camName;

            vector<RgbValue> lowValues(3);
            vector<RgbValue> highValues(3);
            glm::mat3 mixRGB;

            // Get the middle and max values from the previous captures
            for (int c = 0; c < 3; ++c)
            {
                lowValues[c] = params.curves[c][1].second;
                highValues[c] = params.curves[c][_colorCurveSamples - 1].second;
            }

            setObjectAttribute(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

            for (int c = 0; c < 3; ++c)
                for (int otherC = 0; otherC < 3; ++otherC)
                    mixRGB[otherC][c] = (highValues[c][otherC] - lowValues[c][otherC]) / (highValues[otherC][otherC] - lowValues[otherC][otherC]);

            params.mixRGB = glm::inverse(mixRGB);
        }

        //
        // Compute and apply the white balance
        //
        RgbValue targetWhiteBalance = equalizeWhiteBalances();

        for (auto& params : _calibrationParams)
        {
            RgbValue whiteBalance;
            whiteBalance = targetWhiteBalance / params.whiteBalance;
            whiteBalance.normalize();
            Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " Projector " << params.camName << " correction white balance: " << whiteBalance[0] << " / "
                       << whiteBalance[1] << " / " << whiteBalance[2] << Log::endl;

            for (unsigned int c = 0; c < 3; ++c)
                for (auto& v : params.projectorCurves[c])
                    v.second[c] = v.second[c] * whiteBalance[c];

            params.minValues = params.minValues * whiteBalance;
            params.maxValues = params.maxValues * whiteBalance;
        }

        //
        // Get the overall maximum value for rgb(0,0,0), and minimum for rgb(1,1,1)
        //
        // Fourth values contain luminance (calculated using other values)
        vector<float> minValues(4, 0.f);
        vector<float> maxValues(4, numeric_limits<float>::max());
        for (auto& params : _calibrationParams)
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
        for (auto& params : _calibrationParams)
        {
            // We use a common scale and offset to keep color balance
            float range = params.maxValues.luminance() - params.minValues.luminance();
            float offset = (minValues[3] - params.minValues.luminance()) / range;
            float scale = (maxValues[3] - minValues[3]) / range;

            for (unsigned int c = 0; c < 3; ++c)
                for (auto& v : params.projectorCurves[c])
                    v.second[c] = v.second[c] * scale + offset;
        }

        //
        // Send calibration to the cameras
        //
        for (auto& params : _calibrationParams)
        {
            string camName = params.camName;
            Values lut;
            for (unsigned int v = 0; v < 256; ++v)
                for (unsigned int c = 0; c < 3; ++c)
                    lut.push_back(params.projectorCurves[c][v].second[c]);

            auto attrParams = Values();
            attrParams.push_back(lut);
            setObjectAttribute(camName, "colorLUT", attrParams);
            setObjectAttribute(camName, "activateColorLUT", {1});

            Values m(9);
            for (int u = 0; u < 3; ++u)
                for (int v = 0; v < 3; ++v)
                    m[u * 3 + v] = params.mixRGB[u][v];

            attrParams = Values();
            attrParams.push_back(m);
            setObjectAttribute(camName, "colorMixMatrix", attrParams);

            // Also, we set some parameters to default as they interfer with the calibration
            setObjectAttribute(camName, "brightness", {1.0});
            setObjectAttribute(camName, "colorTemperature", {6500.0});
        }

        //
        // Cameras back to normal
        //
        for (auto& params : _calibrationParams)
        {
            setObjectAttribute(params.camName, "hide", {0});
            setObjectAttribute(params.camName, "flashBG", {0});
            setObjectAttribute(params.camName, "clearColor", {});
        }

        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Calibration updated" << Log::endl;
    });
}

/*************/
void ColorCalibrator::updateCRF()
{
    if (!_calibrationMutex.try_lock())
    {
        Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Another calibration is in process. Exiting." << Log::endl;
        return;
    }

    if (_calibrationThread.valid())
        _calibrationThread.wait();

    _calibrationThread = async(launch::async, [&]() {
        // Initialize camera
        _gcamera = make_shared<Image_GPhoto>(_root, "");
        OnScopeExit
        {
            _calibrationMutex.unlock();
            _gcamera.reset();
        };

        // Check whether the camera is ready
        Values status;
        _gcamera->getAttribute("ready", status);
        if (status.size() == 0 || status[0].as<int>() == 0)
        {
            Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Camera is not ready, unable to update color response" << Log::endl;
            return;
        }

        findCorrectExposure();

        // Compute the camera response function
        captureHDR(9, 0.33);
    });
}

/*************/
cv::Mat3f ColorCalibrator::captureHDR(unsigned int nbrLDR, double step, bool computeResponseOnly)
{
    // Capture LDR images
    // Get the current shutterspeed
    Values res;
    _gcamera->getAttribute("shutterspeed", res);
    double defaultSpeed = res[0].as<float>();
    double nextSpeed = defaultSpeed;

    // Compute the parameters of the first capture
    for (int steps = nbrLDR / 2; steps > 0; --steps)
        nextSpeed /= pow(2.0, step);

    vector<cv::Mat> ldr(nbrLDR);
    vector<float> expositionDurations(nbrLDR);
    for (unsigned int i = 0; i < nbrLDR; ++i)
    {
        _gcamera->setAttribute("shutterspeed", {nextSpeed});
        // We get the actual shutterspeed
        _gcamera->getAttribute("shutterspeed", res);
        nextSpeed = res[0].as<float>();
        expositionDurations[i] = nextSpeed;

        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Capturing LDRI with a " << nextSpeed << "sec exposure time" << Log::endl;

        // Update exposure for next step
        nextSpeed *= pow(2.0, step);

        string filename = "/tmp/splash_ldr_sample_" + to_string(i) + ".bmp";
        int status = _gcamera->capture();
        if (status == 0)
        {
            Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Error while capturing LDRI" << Log::endl;
            return {};
        }
        _gcamera->update();
        _gcamera->write(filename);

        ldr[i] = cv::imread(filename);
    }

    // Reset the shutterspeed
    _gcamera->setAttribute("shutterspeed", {defaultSpeed});

    // Check that all is well
    bool isValid = true;
    for (auto& image : ldr)
        isValid |= image.total() != 0;

    if (!isValid)
        return {};

    // Estimate camera response function, if needed
    if (_crf.total() == 0 || computeResponseOnly)
    {
        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Generating camera response function" << Log::endl;
        cv::Ptr<cv::CalibrateDebevec> calibrate = cv::createCalibrateDebevec();
        calibrate->process(ldr, _crf, expositionDurations);
    }

    cv::Mat3f hdr;
    cv::Ptr<cv::MergeDebevec> mergeDebevec = cv::createMergeDebevec();
    mergeDebevec->process(ldr, hdr, expositionDurations, _crf);

    for (uint32_t y = 0; y < hdr.rows; ++y)
        for (uint32_t x = 0; x < hdr.cols; ++x)
        {
            auto pixelValue = hdr.at<cv::Vec3f>(y, x);
            pixelValue[0] = max(0.f, pixelValue[0]);
            pixelValue[1] = max(0.f, pixelValue[1]);
            pixelValue[2] = max(0.f, pixelValue[2]);
        }

    cv::imwrite("/tmp/splash_hdr.hdr", hdr);
    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - HDRI computed" << Log::endl;

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
        // Make sure the points are correctly ordered
        std::sort(curve.begin(), curve.end(), [&](Point a, Point b) { return a.second[c] < b.second[c]; });

        double yOffset = curve[0].second[c];
        double yRange = curve[curve.size() - 1].second[c] - curve[0].second[c];
        if (yRange <= 0.f)
        {
            Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Unable to compute projector inverse function curve on a channel" << Log::endl;
            projInvCurves.push_back(Curve());
            continue;
        }

        vector<double> rawX;
        vector<double> rawY;

        double epsilon = 0.001;
        double previousAbscissa = -1.0;
        for (auto& point : curve)
        {
            double abscissa = (point.second[c] - yOffset) / yRange;
            if (std::abs(abscissa - previousAbscissa) < epsilon)
            {
                Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - Abscissa not strictly increasing: discarding value " << abscissa << " from channel " << c
                           << Log::endl;
            }
            else
            {
                previousAbscissa = abscissa;

                rawX.push_back((point.second[c] - yOffset) / yRange);
                rawY.push_back(point.first);
            }
        }

        // Check that first and last points abscissas are 0 and 1, respectively, and shift them slightly
        // to prevent floating point imprecision to cause an interpolation error
        rawX[0] = std::max(0.0, rawX[0]) - 0.001;
        rawX[rawX.size() - 1] = std::min(1.0, rawX[rawX.size() - 1]) + 0.001;

        gsl_interp_accel* acc = gsl_interp_accel_alloc();
        gsl_spline* spline = nullptr;
        if (rawX.size() > 4)
            spline = gsl_spline_alloc(gsl_interp_akima, rawX.size());
        else
            spline = gsl_spline_alloc(gsl_interp_linear, rawX.size());
        gsl_spline_init(spline, rawX.data(), rawY.data(), rawX.size());

        Curve projInvCurve;
        for (double x = 0.0; x <= 255.0; x += 1.0)
        {
            double realX = std::min(1.0, x / 255.0); // Make sure we don't try to go past 1.0
            Point point;
            point.first = realX;
            point.second[c] = gsl_spline_eval(spline, realX, acc);
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
float ColorCalibrator::findCorrectExposure()
{
    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Finding correct exposure time" << Log::endl;

    Values res;
    while (true)
    {
        _gcamera->getAttribute("shutterspeed", res);
        int status = _gcamera->capture();
        if (status == 0)
        {
            Log::get() << Log::WARNING << "ColorCalibrator::" << __FUNCTION__ << " - There was an issue during capture." << Log::endl;
            return 0.f;
        }

        _gcamera->update();
        ImageBuffer img = _gcamera->get();
        ImageBufferSpec spec = _gcamera->getSpec();

        // Exposure is found from a centered area, covering 4% of the frame
        int roiSize = spec.width / 5;
        unsigned long total = roiSize * roiSize;
        unsigned long sum = 0;

        uint8_t* pixel = reinterpret_cast<uint8_t*>(img.data());
        for (int y = spec.height / 2 - roiSize / 2; y < spec.height / 2 + roiSize / 2; ++y)
            for (int x = spec.width / 2 - roiSize / 2; x < spec.width / 2 + roiSize / 2; ++x)
            {
                auto index = (x + y * spec.width) * spec.channels;
                sum += static_cast<unsigned long>(0.2126 * pixel[index] + 0.7152 * pixel[index + 1] + 0.0722 * pixel[index + 2]);
            }

        auto meanValue = static_cast<float>(sum) / static_cast<float>(total);
        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Mean value over all channels: " << meanValue << Log::endl;

        if (meanValue >= 100.f && meanValue <= 160.f)
            break;

        float speed = 1.f;
        if (meanValue < 100.f)
            speed = res[0].as<float>() * std::max(1.5f, (100.f / meanValue));
        else
            speed = res[0].as<float>() / std::max(1.5f, (160.f / meanValue));
        _gcamera->setAttribute("shutterspeed", {speed});
    }
    if (res.size() == 0)
        return 0.f;
    else
        return res[0].as<float>();
}

/*************/
double computeMoment(const cv::Mat3f& image, int i, int j, double minTargetLum = 0.0, double maxTargetLum = 0.0)
{
    double moment = 0.0;

    for (int y = 0; y < image.rows; ++y)
        for (int x = 0; x < image.cols; ++x)
        {
            auto& pixel = image(y, x);
            double linlum = 0.f;
            for (int c = 0; c < 3; ++c)
                linlum += pixel[c];
            if (minTargetLum == 0.0)
                moment += pow(x, i) * pow(y, j) * linlum;
            else if (maxTargetLum == 0.0 && linlum >= minTargetLum)
                moment += pow(x, i) * pow(y, j);
            else if (linlum >= minTargetLum && linlum <= maxTargetLum)
                moment += pow(x, i) * pow(y, j);
        }

    return moment;
}

/*************/
vector<int> ColorCalibrator::getMaxRegionROI(const cv::Mat3f& image)
{
    if (image.total() == 0)
        return vector<int>();

    vector<int> coords;

    // Find the maximum value
    float maxLinearLuminance = numeric_limits<float>::min();
    for (int y = 0; y < image.rows; ++y)
        for (int x = 0; x < image.cols; ++x)
        {
            const auto& pixel = image(y, x);
            float linlum = pixel[0] + pixel[1] + pixel[2];
            if (linlum > maxLinearLuminance)
                maxLinearLuminance = linlum;
        }

    // Compute the binary moments of all pixels brighter than maxLinearLuminance
    vector<double> moments(3, 0.0);
    double iteration = 0.0;
    while (moments[0] < _minimumROIArea * image.total())
    {
        double minTargetLuminance = maxLinearLuminance / pow(2.0, iteration + 2);
        double maxTargetLuminance = maxLinearLuminance / pow(2.0, iteration);
        moments[0] = computeMoment(image, 0, 0, minTargetLuminance, maxTargetLuminance);
        moments[1] = computeMoment(image, 1, 0, minTargetLuminance, maxTargetLuminance);
        moments[2] = computeMoment(image, 0, 1, minTargetLuminance, maxTargetLuminance);
        iteration += 0.5;
    }

    coords = vector<int>({(int)(moments[1] / moments[0]), (int)(moments[2] / moments[0]), (int)(sqrt(moments[0]) / 2.0)});

    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum found around point (" << coords[0] << ", " << coords[1]
               << ") - Estimated side size: " << coords[2] << Log::endl;

    return coords;
}

/*************/
vector<bool> ColorCalibrator::getMaskROI(const cv::Mat3f& image)
{
    if (image.total() == 0)
        return vector<bool>();

    // Find the maximum value
    float maxLinearLuminance = numeric_limits<float>::min();
    for (int y = 0; y < image.rows; ++y)
        for (int x = 0; x < image.cols; ++x)
        {
            auto& pixel = image(y, x);
            float linlum = pixel[0] + pixel[1] + pixel[2];
            if (linlum > maxLinearLuminance)
                maxLinearLuminance = linlum;
        }

    // Compute the binary moments of all pixels brighter than maxLinearLuminance
    vector<bool> mask;
    unsigned long meanX, meanY;
    double totalPixelMask = 0;
    double iteration = 0.0;
    while (totalPixelMask < _minimumROIArea * image.total())
    {
        totalPixelMask = 0;
        meanX = 0;
        meanY = 0;
        mask = vector<bool>(image.total(), false);

        double minTargetLuminance = maxLinearLuminance / pow(2.0, iteration + 4);
        // double maxTargetLuminance = maxLinearLuminance / pow(2.0, iteration);

        for (int y = 0; y < image.rows; ++y)
            for (int x = 0; x < image.cols; ++x)
            {
                auto& pixel = image(y, x);
                float linlum = pixel[0] + pixel[1] + pixel[2];
                if (linlum > minTargetLuminance && linlum < maxLinearLuminance)
                {
                    mask[y * image.cols + x] = true;
                    meanX += x;
                    meanY += y;
                    totalPixelMask++;
                }
            }

        iteration += 1.0;
    }

    meanX /= totalPixelMask;
    meanY /= totalPixelMask;

    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Region of interest center: [" << meanX << ", " << meanY << "] - Size: " << (int)totalPixelMask
               << Log::endl;

    return mask;
}

/*************/
vector<float> ColorCalibrator::getMeanValue(const cv::Mat3f& image, vector<int> coords, int boxSize)
{
    cv::Scalar meanMaxValue;

    if (coords.size() >= 2)
    {
        cv::Mat1b mask = cv::Mat1b::zeros(image.rows, image.cols);
        for (uint32_t y = coords[1] - boxSize / 2; y < coords[1] + boxSize / 2; ++y)
            for (uint32_t x = coords[0] - boxSize / 2; y < coords[0] + boxSize / 2; ++x)
                mask(y, x) = 255;

        meanMaxValue = cv::mean(image, mask);
    }
    else
    {
        meanMaxValue = cv::mean(image);
    }

    return {static_cast<float>(meanMaxValue[0]), static_cast<float>(meanMaxValue[1]), static_cast<float>(meanMaxValue[2])};
}

/*************/
vector<float> ColorCalibrator::getMeanValue(const cv::Mat3f& image, vector<bool> mask)
{
    vector<float> meanValue(3, 0.f);
    unsigned int nbrPixels = 0;

    if (mask.size() != image.total())
        return vector<float>(3, 0.f);

    for (int y = 0; y < image.rows; ++y)
        for (int x = 0; x < image.cols; ++x)
        {
            if (true == mask[y * image.cols + x])
            {
                meanValue[0] += image(y, x)[0];
                meanValue[1] += image(y, x)[1];
                meanValue[2] += image(y, x)[2];
                nbrPixels++;
            }
        }

    if (nbrPixels == 0)
        return vector<float>(3, 0.f);

    meanValue[0] /= (float)nbrPixels;
    meanValue[1] /= (float)nbrPixels;
    meanValue[2] /= (float)nbrPixels;

    return meanValue;
}

/*************/
RgbValue ColorCalibrator::equalizeWhiteBalancesOnly()
{
    RgbValue whiteBalance;
    float numCameras = 0.f;
    for (auto& params : _calibrationParams)
    {
        params.whiteBalance = params.whitePoint / params.whitePoint[1];
        whiteBalance = whiteBalance + params.whiteBalance;
        numCameras++;

        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " Projector " << params.camName << " initial white balance: " << params.whiteBalance[0] << " / "
                   << params.whiteBalance[1] << " / " << params.whiteBalance[2] << Log::endl;
    }
    whiteBalance = whiteBalance / numCameras;

    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - White balance of the weakest projector: " << whiteBalance[0] << " / " << whiteBalance[1] << " / "
               << whiteBalance[2] << Log::endl;

    return whiteBalance;
}

/*************/
RgbValue ColorCalibrator::equalizeWhiteBalancesFromWeakestLum()
{
    RgbValue minWhiteBalance;
    float minLuminance = numeric_limits<float>::max();
    for (auto& params : _calibrationParams)
    {
        params.whiteBalance = params.whitePoint / params.whitePoint[1];
        if (params.whitePoint.luminance() < minLuminance)
        {
            minLuminance = params.whitePoint.luminance();
            minWhiteBalance = params.whiteBalance;
        }

        Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " Projector " << params.camName << " initial white balance: " << params.whiteBalance[0] << " / "
                   << params.whiteBalance[1] << " / " << params.whiteBalance[2] << Log::endl;
    }

    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - White balance of the weakest projector: " << minWhiteBalance[0] << " / " << minWhiteBalance[1] << " / "
               << minWhiteBalance[2] << Log::endl;

    return minWhiteBalance;
}

/*************/
RgbValue ColorCalibrator::equalizeWhiteBalancesMaximizeMinLum()
{
    RgbValue whiteBalance(1.f, 1.f, 1.f);
    float delta = numeric_limits<float>::max();
    float targetDelta = numeric_limits<float>::max();

    // Target delta is set to 1% of the minimum luminance
    for (auto& params : _calibrationParams)
        targetDelta = std::min(targetDelta, params.whitePoint.luminance() * 0.01f);

    // Get the individual white balances
    for (auto& params : _calibrationParams)
        params.whiteBalance = params.whitePoint / params.whitePoint[1];

    int iteration = 1;
    while (delta > targetDelta)
    {
        // Get the current minimum luminance
        float previousMinLum = numeric_limits<float>::max();
        int minIndex = 0;
        for (int i = 0; i < _calibrationParams.size(); ++i)
        {
            CalibrationParams& params = _calibrationParams[i];
            RgbValue whiteBalanced = params.whitePoint * (params.whiteBalance / whiteBalance).normalize();
            if (whiteBalanced.luminance() < previousMinLum)
            {
                previousMinLum = whiteBalanced.luminance();
                minIndex = i;
            }
        }

        whiteBalance = whiteBalance * 0.5 + _calibrationParams[minIndex].whiteBalance * 0.5;

        // Get the new minimum luminance
        float newMinLum = numeric_limits<float>::max();
        for (auto& params : _calibrationParams)
        {
            RgbValue whiteBalanced = params.whitePoint * (params.whiteBalance / whiteBalance).normalize();
            newMinLum = std::min(newMinLum, whiteBalanced.luminance());
        }

        delta = std::abs(newMinLum - previousMinLum);

#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "ColorCalibrator::" << __FUNCTION__ << " - White balance at iteration " << iteration << ": " << whiteBalance[0] << " / " << whiteBalance[1]
                   << " / " << whiteBalance[2] << " with a delta of " << delta * 100.f / newMinLum << "%" << Log::endl;
#endif
        iteration++;
    }

    Log::get() << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Optimized white balance: " << whiteBalance[0] << " / " << whiteBalance[1] << " / " << whiteBalance[2]
               << Log::endl;

    return whiteBalance;
}

/*************/
void ColorCalibrator::registerAttributes()
{
    ControllerObject::registerAttributes();

    addAttribute("colorSamples",
        [&](const Values& args) {
            _colorCurveSamples = std::max(3, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {(int)_colorCurveSamples}; },
        {'n'});
    setAttributeDescription("colorSamples", "Set the number of color samples");

    addAttribute("detectionThresholdFactor",
        [&](const Values& args) {
            _displayDetectionThreshold = std::max(0.5f, args[0].as<float>());
            return true;
        },
        [&]() -> Values { return {_displayDetectionThreshold}; },
        {'n'});
    setAttributeDescription("detectionThresholdFactor", "Set the threshold for projection detection");

    addAttribute("imagePerHDR",
        [&](const Values& args) {
            _imagePerHDR = std::max(1, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_imagePerHDR}; },
        {'n'});
    setAttributeDescription("imagePerHDR", "Set the number of image per HDRI to shoot");

    addAttribute("hdrStep",
        [&](const Values& args) {
            _hdrStep = std::max(0.3f, args[0].as<float>());
            return true;
        },
        [&]() -> Values { return {_hdrStep}; },
        {'n'});
    setAttributeDescription("hdrStep", "Set the step between two images for HDRI");

    addAttribute("equalizeMethod",
        [&](const Values& args) {
            _equalizationMethod = std::max(0, std::min(2, args[0].as<int>()));
            if (_equalizationMethod == 0)
                equalizeWhiteBalances = std::bind(&ColorCalibrator::equalizeWhiteBalancesOnly, this);
            else if (_equalizationMethod == 1)
                equalizeWhiteBalances = std::bind(&ColorCalibrator::equalizeWhiteBalancesFromWeakestLum, this);
            else if (_equalizationMethod == 2)
                equalizeWhiteBalances = std::bind(&ColorCalibrator::equalizeWhiteBalancesMaximizeMinLum, this);
            return true;
        },
        [&]() -> Values { return {_equalizationMethod}; },
        {'n'});
    setAttributeDescription("equalizeMethod", "Set the color calibration method (0: WB only, 1: WB from weakest projector, 2: WB maximizing minimum luminance");
}

} // end of namespace
