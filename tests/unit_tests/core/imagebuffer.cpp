#include <doctest.h>

#include "./core/imagebuffer.h"

using namespace Splash;

/*************/
TEST_CASE("Testing ImageBufferSpec initialization")
{
    auto spec = ImageBufferSpec(512, 512, 1, 8, ImageBufferSpec::Type::UINT8, "");
    CHECK_EQ(spec.format, "R");
    spec = ImageBufferSpec(512, 512, 2, 16, ImageBufferSpec::Type::UINT8, "");
    CHECK_EQ(spec.format, "RG");
    spec = ImageBufferSpec(512, 512, 3, 24, ImageBufferSpec::Type::UINT8, "");
    CHECK_EQ(spec.format, "RGB");
    spec = ImageBufferSpec(512, 512, 4, 32, ImageBufferSpec::Type::UINT8, "");
    CHECK_EQ(spec.format, "RGBA");
}

/*************/
TEST_CASE("Testing ImageBufferSpec comparison")
{
    auto spec = ImageBufferSpec(512, 512, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");

    auto otherSpec = ImageBufferSpec(1024, 512, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
    CHECK_NE(spec, otherSpec);
    otherSpec = ImageBufferSpec(512, 1024, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
    CHECK_NE(spec, otherSpec);
    otherSpec = ImageBufferSpec(512, 512, 4, 24, ImageBufferSpec::Type::UINT8, "RGB");
    CHECK_NE(spec, otherSpec);
    otherSpec = ImageBufferSpec(512, 512, 3, 32, ImageBufferSpec::Type::UINT8, "RGB");
    CHECK_NE(spec, otherSpec);
    otherSpec = ImageBufferSpec(512, 512, 3, 24, ImageBufferSpec::Type::UINT16, "RGB");
    CHECK_NE(spec, otherSpec);
    otherSpec = ImageBufferSpec(512, 512, 3, 24, ImageBufferSpec::Type::UINT8, "RGBA");
    CHECK_NE(spec, otherSpec);
}

/*************/
TEST_CASE("Testing ImageBufferSpec serialization")
{
    auto spec = ImageBufferSpec(512, 512, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
    CHECK_EQ(spec.to_string(), "512;512;3;24;0;RGB;1;-1;");
    auto otherSpec = ImageBufferSpec();
    otherSpec.from_string(spec.to_string());
    CHECK_EQ(spec, otherSpec);

    spec = ImageBufferSpec(512, 512, 4, 32, ImageBufferSpec::Type::UINT16, "RGBA");
    CHECK_EQ(spec.to_string(), "512;512;4;32;1;RGBA;1;-1;");
    otherSpec = ImageBufferSpec();
    otherSpec.from_string(spec.to_string());
    CHECK_EQ(spec, otherSpec);

    spec = ImageBufferSpec(512, 512, 1, 32, ImageBufferSpec::Type::FLOAT, "R");
    CHECK_EQ(spec.to_string(), "512;512;1;32;2;R;1;-1;");
    otherSpec = ImageBufferSpec();
    otherSpec.from_string(spec.to_string());
    CHECK_EQ(spec, otherSpec);
}

/*************/
TEST_CASE("Testing ImageBuffer")
{
    int width = 128;
    int height = 128;
    int bpp = 32;

    auto spec = ImageBufferSpec(width, height, 4, bpp, ImageBufferSpec::Type::UINT8);
    auto imageBuffer = ImageBuffer(spec);

    SUBCASE("Verifying buffer size")
    {
        CHECK_EQ(imageBuffer.getSize(), width * height * bpp / 8);
    }

    SUBCASE("Verifying buffer after zero-ed")
    {
        auto previousBufferPtr = imageBuffer.data();
        imageBuffer.zero();
        CHECK_EQ(imageBuffer.data(), previousBufferPtr);
    }
}
