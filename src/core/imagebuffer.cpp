#include "./core/imagebuffer.h"

#include <assert.h>
#include <cstring>
#include <vector>

namespace Splash
{

/*************/
std::string ImageBufferSpec::to_string() const
{
    std::string spec;
    spec += std::to_string(width) + ";";
    spec += std::to_string(height) + ";";
    spec += std::to_string(channels) + ";";
    spec += std::to_string(bpp) + ";";

    switch (type)
    {
    default:
        assert(false);
        break;
    case Type::UINT8:
        spec += "0;";
        break;
    case Type::UINT16:
        spec += "1;";
        break;
    case Type::FLOAT:
        spec += "2;";
        break;
    }
    spec += format + ";";
    spec += std::to_string(static_cast<int>(videoFrame)) + ";";
    spec += std::to_string(timestamp) + ";";

    return spec;
}

/*************/
void ImageBufferSpec::from_string(const std::string& spec)
{
    std::vector<std::string> parts;

    size_t prev = 0;
    size_t curr = spec.find(";");
    while (curr != std::string::npos)
    {
        parts.push_back(spec.substr(prev, curr - prev));
        prev = curr + 1;
        curr = spec.find(";", prev);
    }
    assert(parts.size() == 8);

    width = stoi(parts[0]);
    height = stoi(parts[1]);
    channels = stoi(parts[2]);
    bpp = stoi(parts[3]);
    switch (stoi(parts[4]))
    {
    default:
        assert(false);
        break;
    case 0:
        type = Type::UINT8;
        break;
    case 1:
        type = Type::UINT16;
        break;
    case 2:
        type = Type::FLOAT;
        break;
    }
    format = parts[5];
    videoFrame = static_cast<bool>(stoi(parts[6]));
    timestamp = stoll(parts[7]);
}

/*************/
ImageBuffer::ImageBuffer(const ImageBufferSpec& spec, uint8_t* data, bool map)
{
    assert(data || (!data && !map));
    _spec = spec;

    if (data && map)
    {
        _mappedBuffer = data;
    }
    else if (!map)
    {
        const auto size = spec.rawSize();
        if (data)
            _buffer = ResizableArray<uint8_t>(data, data + size);
        else
            _buffer.resize(size);
    }
}

/*************/
void ImageBuffer::zero()
{
    if (_mappedBuffer)
        return;
    if (_buffer.size())
        memset(_buffer.data(), 0, _buffer.size());
}

} // end of namespace
