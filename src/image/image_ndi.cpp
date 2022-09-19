#include "./image/image_ndi.h"

#include "./core/root_object.h"

using Splash::Utils::Subprocess;

namespace Splash
{

/*************/
Image_NDI::Image_NDI(RootObject* root)
    : Image(root)
    , _shmdata(root)
{
    _type = "image_ndi";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/*************/
Image_NDI::~Image_NDI()
{
    _subprocess.reset();

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Image_NDI::~Image_NDI - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_NDI::read(const std::string& sourceName)
{
    if (_root == nullptr)
        return false;

    if (sourceName.empty())
        return false;

    const auto prefix = _root->getSocketPrefix();
    const auto shmpath = "/tmp/splash_ndi_" + prefix + "_" + _name;
    const auto args = "-n \"" + sourceName + "\" -v " + shmpath + " -f";
    _subprocess = std::make_unique<Subprocess>("ndi2shmdata", args);

    if (!_subprocess->isRunning())
    {
        _subprocess.reset();
        _filepath.clear();
        return false;
    }

    _shmdata.read(shmpath);

    return true;
}

/*************/
void Image_NDI::update()
{
    _shmdata.update();
    if (_bufferImage == nullptr)
        _bufferImage = std::make_unique<ImageBuffer>();
    *_bufferImage = _shmdata.get();
    _bufferImageUpdated = true;
    updateTimestamp(_bufferImage->getSpec().timestamp);

    Image::update();
}

/*************/
void Image_NDI::registerAttributes()
{
    Image::registerAttributes();

    setAttributeDescription("file", R"(NDI source name to read.
Note that you need to have ndi2shmdata installed to read from NDI sources.
Get it at https://gitlab.com/sat-mtl/tools/ndi2shmdata)");
}

} // namespace Splash
