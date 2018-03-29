#include "./utils/cgutils.h"

#include <future>

using namespace std;

namespace Splash
{

/*************/
void hapDecodeCallback(HapDecodeWorkFunction func, void* p, unsigned int count, void* /*info*/)
{
    vector<future<void>> threads;
    for (unsigned int i = 0; i < count; ++i)
        threads.push_back(async(launch::async, [=]() { func(p, i); }));
}

/*************/
bool hapDecodeFrame(void* in, unsigned int inSize, void* out, unsigned int outSize, std::string& format)
{
    // We are using kind of a hack to store a DXT compressed image in an ImageBuffer
    // First, we check the texture format type
    unsigned int textureFormat = 0;
    if (HapGetFrameTextureFormat(in, inSize, 0, &textureFormat) != HapResult_No_Error)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Unknown texture format. Frame discarded" << Log::endl;
        return false;
    }

    if (textureFormat == HapTextureFormat_RGB_DXT1)
        format = "RGB_DXT1";
    else if (textureFormat == HapTextureFormat_RGBA_DXT5)
        format = "RGBA_DXT5";
    else if (textureFormat == HapTextureFormat_YCoCg_DXT5)
        format = "YCoCg_DXT5";
    else
        return false;

    if (out == nullptr)
        return true;

    unsigned long bytesUsed = 0;
    if (HapDecode(in, inSize, 0, hapDecodeCallback, nullptr, out, outSize, &bytesUsed, &textureFormat) != HapResult_No_Error)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - An error occured while decoding frame" << Log::endl;
        return false;
    }

    return true;
}

} // end of namespace
