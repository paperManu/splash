#include "./imageBuffer.h"

using namespace std;

namespace Splash {

/*************/
string ImageBufferSpec::to_string()
{
    string spec;
    spec += std::to_string(width);
    spec += ";";
    spec += std::to_string(height);
    spec += ";";
    spec += std::to_string(channels);
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

    spec += std::to_string(format.size());
    spec += ";";

    for (auto& c : format)
    {
        spec += c;
        spec += ";";
    }

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

    // Type
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    if (curr == string::npos)
        return;
    switch (stoi(spec.substr(prev, curr)))
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

    // Format descriptor size
    roi = roi.substr(curr + 1);
    curr = roi.find(";");
    if (curr == string::npos)
        return;
    int formatSize = stoi(roi.substr(0, curr));

    // Format
    format.clear();
    for (uint32_t pos = 0; pos < formatSize; ++pos)
    {
        roi = roi.substr(curr + 1);
        curr = roi.find(";");
        format.push_back(roi.substr(0, curr));
    }
}

/*************/
ImageBuffer::ImageBuffer()
{
}

/*************/
ImageBuffer::ImageBuffer(const ImageBufferSpec& spec)
{
    init(spec);
}

/*************/
ImageBuffer::ImageBuffer(unsigned int width, unsigned int height, unsigned int channels, ImageBufferSpec::Type type)
{
    auto spec = ImageBufferSpec();
    spec.width = width;
    spec.height = height;
    spec.channels = channels;
    spec.type = type;
    
    switch (channels)
    {
    case 1:
        spec.format = {"R"};
        break;
    case 2:
        spec.format = {"R", "G"};
        break;
    case 3:
        spec.format = {"R", "G", "B"};
        break;
    case 4:
        spec.format = {"R", "G", "B", "A"};
        break;
    }
}

/*************/
ImageBuffer::~ImageBuffer()
{
}

/*************/
void ImageBuffer::init(const ImageBufferSpec& spec)
{
    _spec = spec;

    uint32_t size = spec.width * spec.height * spec.channels;
    switch (spec.type)
    {
    case ImageBufferSpec::Type::UINT8:
        break;
    case ImageBufferSpec::Type::UINT16:
        size *= 2;
        break;
    case ImageBufferSpec::Type::FLOAT:
        size *= 4;
        break;
    }

    _buffer.resize(size);
    fill(0.f);
}

/*************/
void ImageBuffer::fill(float value)
{
    size_t size = 1;
    switch (_spec.type)
    {
    case ImageBufferSpec::Type::UINT8:
        {
            uint8_t* data = static_cast<uint8_t*>(_buffer.data());
            uint8_t v = static_cast<uint8_t>(value);
            for (uint32_t p = 0; p < _spec.width * _spec.height * _spec.channels; ++p)
                data[p] = v;
        }
        break;
    case ImageBufferSpec::Type::UINT16:
        {
            uint16_t* data = reinterpret_cast<uint16_t*>(_buffer.data());
            uint16_t v = static_cast<uint16_t>(value);
            for (uint32_t p = 0; p < _spec.width * _spec.height * _spec.channels; ++p)
                data[p] = v;
        }
        break;
    case ImageBufferSpec::Type::FLOAT:
        {
            float* data = reinterpret_cast<float*>(_buffer.data());
            for (uint32_t p = 0; p < _spec.width * _spec.height * _spec.channels; ++p)
                data[p] = value;
        }
        break;
    }
}

} // end of namespace
