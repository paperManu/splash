#include "./sink_shmdata.h"

using namespace std;

namespace Splash
{

/*************/
Sink_Shmdata::Sink_Shmdata(RootObject* root)
    : Sink(root)
{
    _type = "sink_shmdata";
    registerAttributes();
}

/*************/
void Sink_Shmdata::handlePixels(const char* pixels, const ImageBufferSpec& spec)
{
    auto size = spec.rawSize();
    if (!pixels || size == 0)
        return;

    if (!_writer || spec != _previousSpec || _framerate != _previousFramerate)
    {
        _previousSpec = spec;
        _previousFramerate = _framerate;
        _caps = "video/x-raw,format=(string)" + spec.format + ",width=(int)" + to_string(spec.width) + ",height=(int)" + to_string(spec.height) + ",framerate=(fraction)" +
                to_string(_framerate) + "/1,pixel-aspect-ratio=(fraction)1/1";
        _writer.reset(nullptr);
        _writer.reset(new shmdata::Writer(_path, size, _caps, &_logger));
    }

    if (_writer)
        _writer->copy_to_shm(pixels, size);
}

/*************/
void Sink_Shmdata::registerAttributes()
{
    Sink::registerAttributes();

    addAttribute("socket",
        [&](const Values& args) {
            _path = args[0].as<string>();
            _previousSpec = ImageBufferSpec();
            return true;
        },
        [&]() -> Values { return {_path}; },
        {'s'});
    setAttributeDescription("socket", "Socket path to which data is sent");

    addAttribute("caps", [&](const Values& args) { return true; }, [&]() -> Values { return {_caps}; }, {'s'});
    setAttributeDescription("caps", "Caps of the sent data");
}

} // end of namespace
