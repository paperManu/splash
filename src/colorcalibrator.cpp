#include "colorcalibrator.h"

#include "image_gphoto.h"
#include "world.h"

using namespace std;

namespace Splash
{

/*************/
ColorCalibrator::ColorCalibrator(WorldWeakPtr world)
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

    _gcamera->capture();
}

/*************/
void ColorCalibrator::registerAttributes()
{
}

} // end of namespace
