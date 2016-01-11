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

    for (auto& c : format)
        spec += c;
    return spec;
}

/*************/
void ImageBufferSpec::from_string(const string& spec)
{
    auto prev = 0;

    auto curr = spec.find(";");
    if (curr == string::npos)
        return;
    width = stoi(spec.substr(prev, curr));

    prev = curr + 1;
    curr = spec.find(";");
    if (curr == string::npos)
        return;
    height = stoi(spec.substr(prev, curr));

    prev = curr + 1;
    curr = spec.find(";");
    if (curr == string::npos)
        return;
    channels = stoi(spec.substr(prev, curr));

    prev = curr + 1;
    curr = spec.find(";");
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

    prev = curr + 1;
    format.clear();
    for (uint32_t pos = prev; pos < spec.size(); ++pos)
        format.push_back(spec.substr(pos, pos + 1));
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
}

} // end of namespace
