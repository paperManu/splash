#include "./core/imagebuffer.h"

#include <assert.h>

using namespace std;

namespace Splash
{

/*************/
string ImageBufferSpec::to_string() const
{
    string spec;
    spec += std::to_string(width);
    spec += ";";
    spec += std::to_string(height);
    spec += ";";
    spec += std::to_string(channels);
    spec += ";";
    spec += std::to_string(bpp);
    spec += ";";

    switch (type)
    {
    case Type::UINT8:
        spec += "0";
        break;
    case Type::UINT16:
        spec += "1";
        break;
    case Type::FLOAT:
        spec += "2";
        break;
    }
    spec += ";";
    spec += format;
    spec += ";";
    spec += std::to_string(static_cast<int>(videoFrame));
    spec += ";";

    return spec;
}

/*************/
void ImageBufferSpec::from_string(const string& spec)
{
    auto prev = 0;

    // Width
    auto curr = spec.find(";");
    if (curr == string::npos)
        return;
    width = stoi(spec.substr(prev, curr));

    // Height
    auto roi = spec.substr(curr + 1);
    curr = roi.find(";");
    if (curr == string::npos)
        return;
    height = stoi(roi.substr(0, curr));

    // Channels
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    if (curr == string::npos)
        return;
    channels = stoi(roi.substr(0, curr));

    // Bits per pixel
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    if (curr == string::npos)
        return;
    bpp = stoi(roi.substr(0, curr));

    // Type
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    if (curr == string::npos)
        return;
    switch (stoi(roi.substr(prev, curr)))
    {
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

    // Format
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    format = roi.substr(0, curr);

    // Video frame
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    videoFrame = static_cast<bool>(stoi(roi.substr(0, curr)));
}

/*************/
ImageBuffer::ImageBuffer(const ImageBufferSpec& spec, char* data, bool map)
{
    assert(data || (!data && !map));
    _spec = spec;

    if (data && map)
    {
        _mappedBuffer = data;
    }
    else if (!map)
    {
        auto size = spec.width * spec.height * spec.pixelBytes();
        if (data)
            _buffer = ResizableArray<char>(data, data + size);
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
