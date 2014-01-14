#include <bandit/bandit.h>

#include "image.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Image class", [&]() {
        Image image;
        before_each([&]() {
            image = Image();
        });

        it("should get the same image specs", [&]() {
            ImageBuf srcImg = image.get();
            SerializedObject obj = image.serialize();
            image.deserialize(obj);
            ImageBuf dstImg = image.get();

            ImageSpec src = srcImg.spec();
            ImageSpec dst = dstImg.spec();
            bool isIdentical = true;
            if (src.width != dst.width)
                isIdentical = false;
            if (src.height != dst.height)
                isIdentical = false;
            if (src.nchannels != dst.nchannels)
                isIdentical = false;
            if (src.format != dst.format)
                isIdentical = false;

            AssertThat(isIdentical, Equals(true));
        });

        it("should get the same image content", [&]() {
            ImageBuf srcImg = image.get();
            SerializedObject obj = image.serialize();

            Image newImage;
            newImage.deserialize(obj);
            ImageBuf dstImg = newImage.get();

            bool isIdentical = true;
            if (srcImg.nchannels() == dstImg.nchannels())
            {
                for (ImageBuf::Iterator<unsigned char> s(srcImg), d(dstImg); !s.done(), !d.done(); ++s, ++d)
                {
                    if (!s.exists() || !d.exists())
                        continue;
                    for (int c = 0; c < srcImg.nchannels(); ++c)
                    {
                        if (s[c] != d[c])
                            isIdentical = false;
                    }
                }
            }
            else
                isIdentical = false;

            AssertThat(isIdentical, Equals(true));
        });
    });
});

/*************/
int main(int argc, char** argv)
{
    SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
