#include "image.h"

using namespace std;

namespace Splash {

/*************/
Image::Image()
{
    _type = "image";

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
    int imgSize = _image.spec().pixel_bytes() * _image.spec().width * _image.spec().height;
    obj.resize(sizeof(nbrChar) + nbrChar + imgSize);

    auto currentObjPtr = obj.data();
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&nbrChar);
    copy(ptr, ptr + sizeof(nbrChar), currentObjPtr);
    currentObjPtr += sizeof(nbrChar);

    const unsigned char* charPtr = reinterpret_cast<const unsigned char*>(xmlSpec.c_str());
    copy(charPtr, charPtr + nbrChar, currentObjPtr);
    currentObjPtr += nbrChar;

    // And then, the image
    const unsigned char* imgPtr = reinterpret_cast<const unsigned char*>(_image.localpixels());
    if (imgPtr == NULL)
        return SerializedObject();
    copy(imgPtr, imgPtr + imgSize, currentObjPtr);

    return obj;
}

/*************/
bool Image::deserialize(const Image::SerializedObject& obj)
{
    if (obj.size() == 0)
        return false;

    // First, we get the size of the metadata
    int nbrChar;
    unsigned char* ptr = reinterpret_cast<unsigned char*>(&nbrChar);

    auto currentObjPtr = obj.data();
    copy(currentObjPtr, currentObjPtr + sizeof(nbrChar), ptr);
    currentObjPtr += sizeof(nbrChar);

    try
    {
        char xmlSpecChar[nbrChar];
        ptr = reinterpret_cast<unsigned char*>(xmlSpecChar);
        copy(currentObjPtr, currentObjPtr + nbrChar, ptr);
        currentObjPtr += nbrChar;
        string xmlSpec(xmlSpecChar);

        ImageSpec spec;
        spec.from_xml(xmlSpec.c_str());

        ImageBuf image(spec);
        int imgSize = image.spec().pixel_bytes() * image.spec().width * image.spec().height;
        ptr = reinterpret_cast<unsigned char*>(image.localpixels());
        copy(currentObjPtr, currentObjPtr + imgSize, ptr);

        _image.swap(image);
    }
    catch (...)
    {
        SLog::log << Log::ERROR << "Image::" << __FUNCTION__ << " - Unable to deserialize the given object" << Log::endl;
        return false;
    }

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
