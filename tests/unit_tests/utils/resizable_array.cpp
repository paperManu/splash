#include <doctest.h>

#include <cstring>
#include <utility>
#include <vector>

#include "./utils/resizable_array.h"

using namespace Splash;

/*************/
int checkResizeUp(int size)
{
    auto array = ResizableArray<uint8_t>(size);
    array.resize(size * 2);
    return array.size();
}

int checkResizeDown(int size)
{
    auto array = ResizableArray<uint8_t>(size);
    array.resize(size / 2);
    return array.size();
}

TEST_CASE("Testing ResizableArray resize")
{
    auto array = ResizableArray<uint8_t>(256);
    array.resize(0);
    CHECK_EQ(array.size(), 0);

    for (int s = 10; s < 1e8; s *= 10)
        CHECK_EQ(checkResizeUp(s), s * 2);

    for (int s = 1e8; s > 10; s /= 10)
        CHECK_EQ(checkResizeDown(s), s / 2);
}

/*************/
int checkShift(int size, int shift)
{
    auto array = ResizableArray<uint8_t>(size);
    array.shift(shift);
    return array.size();
}

TEST_CASE("Testing ResizableArray shift")
{
    for (int size = 1000; size < 1e8; size *= 10)
        for (int shift = 100; shift < 500; shift += 100)
            CHECK(checkShift(size, shift) == size - shift);
}

/*************/
int checkCopy(int size, int shift)
{
    auto array = ResizableArray<uint8_t>(size);
    array.shift(shift);
    memset(array.data(), 0, array.size());

    // Copy constructor
    auto newArray(array);
    if (array.size() != newArray.size())
        return -1;

    for (uint32_t i = 0; i < newArray.size(); ++i)
        if (newArray[i] != array[i])
            return -2;

    // Copy operator
    newArray = array;
    if (array.size() != newArray.size())
        return -3;

    for (uint32_t i = 0; i < newArray.size(); ++i)
        if (newArray[i] != array[i])
            return -4;

    return newArray.size();
}

TEST_CASE("Testing ResizableArray copy")
{
    for (int size = 1000; size < 1e8; size *= 10)
        for (int shift = 100; shift < 500; shift += 100)
            CHECK(checkCopy(size, shift) == size - shift);
}

/*************/
TEST_CASE("Testing ResizableArray constructor from iterators")
{
    std::vector<int> data(256, 42);
    auto array = ResizableArray(&data[0], &data[255]);
    CHECK_EQ(array[128], 42);
}

/*************/
TEST_CASE("Testing ResizableArray move constructor and operator")
{
    std::vector<int> data(256, 42);
    auto array = ResizableArray(&data[0], &data[255]);

    auto otherArray = ResizableArray(std::move(array));
    CHECK_EQ(otherArray[128], 42);

    ResizableArray<int> anotherOne;
    anotherOne = std::move(otherArray);
    CHECK_EQ(anotherOne[128], 42);
}
