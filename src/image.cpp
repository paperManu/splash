#include "image.h"

using namespace std;

namespace Splash {

/*************/
Image::Image()
{
    createDefaultImage();
}

/*************/
Image::~Image()
{
}

/*************/
ImageBuf Image::get() const
{
    ImageBuf img(_image);
    return img;
}

/*************/
void Image::set(const ImageBuf& img)
{
    _image.copy(img);
}

/*************/
Image::SerializedObject Image::serialize() const
{
    SerializedObject obj;

    // We first get the xml version of the specs, and pack them into the obj
    string xmlSpec = _image.spec().to_xml();
    int nbrChar = xmlSpec.size();
    const unsigned char* charPtr = reinterpret_cast<const unsigned char*>(xmlSpec.c_str());

    return SerializedObject();
}

/*************/
bool Image::deserialize(const Image::SerializedObject& obj)
{
    return true;
}

/*************/
void Image::createDefaultImage()
{
    ImageSpec spec(512, 512, 3, TypeDesc::UINT8);
    ImageBuf img(spec);

    int active = 0;
    for (ImageBuf::Iterator<unsigned char> p(img); !p.done(); ++p)
    {
        active = (active + 1) % 2;
        if (!p.exists() || !active)
            continue;
        for (int c = 0; c < img.nchannels(); ++c)
            p[c] = 255;
    }

    _image.swap(img);
}

} // end of namespace
