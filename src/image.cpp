#include "image.h"

#include <OpenImageIO/imageio.h>

using namespace std;

namespace Splash {

/*************/
Image::Image()
{
    _type = "image";

    createDefaultImage();

    registerAttributes();
}

/*************/
Image::~Image()
{
}

/*************/
const void* Image::data() const
{
    return _image.localpixels();
}

/*************/
ImageBuf Image::get() const
{
    ImageBuf img;
    img.copy(_image);
    return img;
}

/*************/
ImageSpec Image::getSpec() const
{
    return _image.spec();
}

/*************/
void Image::set(const ImageBuf& img)
{
    _image.copy(img);
}

/*************/
SerializedObject Image::serialize() const
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
bool Image::deserialize(const SerializedObject& obj)
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

        updateTimestamp();
    }
    catch (...)
    {
        SLog::log << Log::ERROR << "Image::" << __FUNCTION__ << " - Unable to deserialize the given object" << Log::endl;
        return false;
    }

    return true;
}

/*************/
bool Image::read(const string& filename)
{
    ImageInput* in = ImageInput::open(filename);
    if (!in)
    {
        SLog::log << Log::WARNING << "Image::" << __FUNCTION__ << " - Unable to load file " << filename << Log::endl;
        return false;
    }

    const ImageSpec& spec = in->spec();
    if (spec.format != TypeDesc::UINT8)
    {
        SLog::log << Log::WARNING << "Image::" << __FUNCTION__ << " - Only 8bit images are supported." << Log::endl;
        return false;
    }

    int xres = spec.width;
    int yres = spec.height;
    int channels = spec.nchannels;
    ImageBuf img(spec); 
    in->read_image(TypeDesc::UINT8, img.localpixels());

    in->close();
    delete in;

    _image.swap(img);
    updateTimestamp();

    return true;
}

/*************/
void Image::createDefaultImage()
{
    ImageSpec spec(512, 512, 3, TypeDesc::UINT8);
    ImageBuf img(spec);

    for (ImageBuf::Iterator<unsigned char> p(img); !p.done(); ++p)
    {
        if (!p.exists())
            continue;

        if (p.x() % 16 > 7 && p.y() % 64 > 31)
            for (int c = 0; c < img.nchannels(); ++c)
                p[c] = 255;
        else
            for (int c = 0; c < img.nchannels(); ++c)
                p[c] = 0;
    }

    _image.swap(img);
    updateTimestamp();
}

/*************/
void Image::updateTimestamp()
{
    _timestamp = chrono::high_resolution_clock::now();
}

/*************/
void Image::registerAttributes()
{
    _attribFunctions["file"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

} // end of namespace
