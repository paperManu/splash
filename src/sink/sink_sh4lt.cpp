#include "./sink/sink_sh4lt.h"

#include <sh4lt/shtype/shtype-from-gst-caps.hpp>

namespace Splash
{

/*************/
Sink_Sh4lt::Sink_Sh4lt(RootObject* root)
    : Sink(root)
{
    _type = "sink_sh4lt";
    registerAttributes();
}

/*************/
void Sink_Sh4lt::handlePixels(const char* pixels, const ImageBufferSpec& spec)
{
    auto size = spec.rawSize();
    if (!pixels || size == 0)
        return;

    if (!_writer || spec != _previousSpec || _framerate != _previousFramerate)
    {
        _previousSpec = spec;
        _previousFramerate = _framerate;
        _shtype = sh4lt::shtype::shtype_from_gst_caps(getCaps(), _label, _group);
        _writer.reset();
        _writer = std::make_unique<sh4lt::Writer>(_shtype, size, &_logger);
    }

    if (_writer)
    {
        // Sh4lt timestamp is expressed in nanoseconds, while Splash in microseconds
        _writer->copy_to_shm(pixels, size, 1000 * spec.timestamp, 1000000000 / _framerate);
    }
}

/*************/
void Sink_Sh4lt::registerAttributes()
{
    Sink::registerAttributes();
    addAttribute(
        "group",
        [&](const Values& args) {
            _group = args[0].as<std::string>();
            _previousSpec = ImageBufferSpec();
            return true;
        },
        [&]() -> Values { return {_group}; },
        {'s'});
    setAttributeDescription("group", "Sh4lt group label");

    addAttribute(
        "label",
        [&](const Values& args) {
            _label = args[0].as<std::string>();
            _previousSpec = ImageBufferSpec();
            return true;
        },
        [&]() -> Values { return {_label}; },
        {'s'});
    setAttributeDescription("label", "Sh4lt group label");

    addAttribute(
        "sh4lt type", [&](const Values&) { return true; }, [&]() -> Values { return {sh4lt::shtype::shtype_to_gst_caps(_shtype)}; }, {'s'});
    setAttributeDescription("sh4lt type", "format of the data sent to the Sh4lt");
}

} // namespace Splash
