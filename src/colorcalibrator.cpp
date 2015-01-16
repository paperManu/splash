#include "colorcalibrator.h"

#define PIC_DISABLE_OPENGL
#define PIC_DISABLE_QT

#include <piccante.hpp>
#include <utility>

#include "image_gphoto.h"
#include "log.h"
#include "timer.h"
#include "threadpool.h"
#include "world.h"

using namespace std;

namespace Splash
{

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
    // Compute the camera response function
    _nbrImageHdr = 5;
    captureHDR();
    _nbrImageHdr = 3;

    // Get the Camera list
    Values cameraList = world->getObjectsNameByType("camera");
    struct CalibrationParams
    {
        vector<int> camPos {2};
        vector<float> minValues {3};
        vector<float> maxValues {3};
        vector<pair<float, vector<float>>> curves {3};
    };
    vector<CalibrationParams> calibrationParams;

    // All cameras to black
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {1});
        world->sendMessage(cam.asString(), "flashBG", {1});
        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});
    }

    // Measure the min and max for each Camera
    // and find the minimum and maximum for all of them
    shared_ptr<pic::Image> hdr;
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "clearColor", {1.0, 1.0, 1.0, 1.0});
        hdr = captureHDR();
        vector<int> coords = getMaxRegionCenter(hdr);
        vector<float> maxValues = getMeanValue(hdr, coords);
        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum values: " << maxValues[0] << " - " << maxValues[1] << " - " << maxValues[2] << Log::endl;

        world->sendMessage(cam.asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});
        hdr = captureHDR();
        vector<float> minValues = getMeanValue(hdr, coords);
        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Minimum values: " << minValues[0] << " - " << minValues[1] << " - " << minValues[2] << Log::endl;

        // Save the camera center for later use
        CalibrationParams params;
        params.camPos = coords;
        params.minValues = minValues;
        params.maxValues = maxValues;
        calibrationParams.push_back(params);
    }

    // Find minimum and maximum luminance values
    vector<float> minValues(3, 0.f);
    vector<float> maxValues(3, numeric_limits<float>::max());
    for (auto& params : calibrationParams)
    {
        minValues = std::max(minValues, params.minValues);
        maxValues = std::min(maxValues, params.maxValues);
    }

    cout << "~~~~> " << minValues[0] << " - " << minValues[1] << " - " << minValues[2] << endl;
    cout << "~~~~> " << maxValues[0] << " - " << maxValues[1] << " - " << maxValues[2] << endl;

    // Find color curves for each Camera
    for (unsigned int i = 0; i < cameraList.size(); ++i)
    {
        string camName = cameraList[i].asString();
        CalibrationParams params = calibrationParams[i];

        for (float r = 0.0; r <= 1.0; r += 0.1)
        {
            world->sendMessage(camName, "clearColor", {r, 0.0, 0.0, 1.0});
            hdr = captureHDR();
            vector<float> values = getMeanValue(hdr, params.camPos);
            world->sendMessage(camName, "clearColor", {0.0, 0.0, 0.0, 1.0});

            SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - RGB values: " << values[0] << " - " << values[1] << " - " << values[2] << Log::endl;

            params.curves.push_back(pair<float, vector<float>>(r, values));
        }
    }

    // Cameras back to normal
    for (auto& cam : cameraList)
    {
        world->sendMessage(cam.asString(), "hide", {0});
        world->sendMessage(cam.asString(), "flashBG", {0});
        world->sendMessage(cam.asString(), "clearColor", {});
    }
    
    _gcamera.reset();
    _crf.reset();
}

/*************/
shared_ptr<pic::Image> ColorCalibrator::captureHDR()
{
    // TODO: find the optimal exposure. Currently we assume the camera is set correctly
    // (same goes for focus and ISO settings)
    // Capture LDR images
    // Get the current shutterspeed
    Values res;
    _gcamera->getAttribute("shutterspeed", res);
    float defaultSpeed = res[0].asFloat();
    float nextSpeed = defaultSpeed;

    // Compute the parameters of the first capture
    for (int steps = _nbrImageHdr / 2; steps > 0; --steps)
        nextSpeed /= 2.f;

    vector<pic::Image> ldr(_nbrImageHdr);
    vector<float> actualShutterSpeeds(_nbrImageHdr);
    for (unsigned int i = 0; i < _nbrImageHdr; ++i)
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
        nextSpeed *= 2.f;
    }

    // Reset the shutterspeed
    _gcamera->setAttribute("shutterspeed", {defaultSpeed});

    // Check that all is well
    bool isValid = true;
    for (auto& image : ldr)
        isValid |= image.isValid();

    // Align all exposures on the well-exposed one
    int medianLDRIndex = _nbrImageHdr / 2 + 1;
    vector<Eigen::Vector2i> shift(_nbrImageHdr);
    for (unsigned int i = 0; i < _nbrImageHdr; ++i)
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

    for (unsigned int i = 0; i < _nbrImageHdr; ++i)
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
    
    pic::BBox box(coords[0] - _meanBoxSize / 2, coords[0] + _meanBoxSize / 2,
                  coords[1] - _meanBoxSize / 2, coords[1] + _meanBoxSize / 2);

    image->getMeanVal(&box, meanMaxValue.data());

    return meanMaxValue;
}

/*************/
void ColorCalibrator::registerAttributes()
{
}

} // end of namespace
