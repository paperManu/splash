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
    _gcamera = make_shared<Image_GPhoto>();
}

/*************/
ColorCalibrator::~ColorCalibrator()
{
}

/*************/
void ColorCalibrator::update()
{
    auto world = _world.lock();
    // Get the Camera list
    Values cameraList = world->getObjectsNameByType("camera");

    _nbrImageHdr = 5;
    captureHDR();
    _nbrImageHdr = 3;

    world->sendMessage(cameraList[1].asString(), "hide", {1});
    world->sendMessage(cameraList[1].asString(), "flashBG", {1});

    world->sendMessage(cameraList[1].asString(), "clearColor", {1.0, 1.0, 1.0, 1.0});
    shared_ptr<pic::Image> hdr = captureHDR();
    vector<int> coords;
    vector<float> maxValue = getMeanMaxValue(hdr, coords);
    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum value: " << maxValue[0] << " - " << maxValue[1] << " - " << maxValue[2] << Log::endl;

    world->sendMessage(cameraList[1].asString(), "clearColor", {0.0, 0.0, 0.0, 1.0});
    hdr = captureHDR();
    maxValue = getMeanMaxValue(hdr, coords);
    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum value: " << maxValue[0] << " - " << maxValue[1] << " - " << maxValue[2] << Log::endl;

    for (float r = 0.0; r <= 1.0; r += 0.1)
    {
        world->sendMessage(cameraList[1].asString(), "clearColor", {r, 0.0, 0.0, 1.0});
        hdr = captureHDR();
        maxValue = getMeanMaxValue(hdr, coords);
        SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum value: " << maxValue[0] << " - " << maxValue[1] << " - " << maxValue[2] << Log::endl;
    }

    world->sendMessage(cameraList[1].asString(), "clearColor", {});
    world->sendMessage(cameraList[1].asString(), "hide", {0});
    world->sendMessage(cameraList[1].asString(), "flashBG", {0});
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
vector<float> ColorCalibrator::getMeanMaxValue(shared_ptr<pic::Image> image, vector<int>& coords)
{
    if (image == nullptr || !image->isValid())
        return vector<float>();

    vector<float> meanMaxValue(image->channels, numeric_limits<float>::min());
    float maxLinearLuminance = numeric_limits<float>::min();
    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
        {
            float* pixel = (*image)(x, y);
            float linlum = 0.f;
            for (int c = 0; c < image->channels; ++c)
                linlum += pixel[c];
            if (linlum > maxLinearLuminance)
            {
                maxLinearLuminance = linlum;
                coords = vector<int>({x, y});
            }
        }

    SLog::log << Log::MESSAGE << "ColorCalibrator::" << __FUNCTION__ << " - Maximum found around point (" << coords[0] << ", " << coords[1] << ")" << Log::endl;

    pic::BBox box(coords[0] - _meanBoxSize / 2, coords[0] + _meanBoxSize / 2,
                  coords[1] - _meanBoxSize / 2, coords[1] + _meanBoxSize / 2);

    image->getMeanVal(&box, meanMaxValue.data());

    return meanMaxValue;
}

/*************/
void ColorCalibrator::getIntegersFromFraction(std::string fraction, int& num, int& denom)
{
    num = 1;
    denom = 1;
    if (fraction.find("1/") != string::npos)
        denom = stoi(fraction.substr(2));
    else
        num = stoi(fraction);
}

/*************/
string ColorCalibrator::getFractionFromIntegers(int num, int denom)
{
    return to_string(num) + "/" + to_string(denom);
}

/*************/
void ColorCalibrator::registerAttributes()
{
}

} // end of namespace
