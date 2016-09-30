#include <doctest.h>

#include "./splash.h"

using namespace std;
using namespace Splash;

/*************/
int checkResize(int size)
{
    auto array = ResizableArray<uint8_t>(size);
    array.resize(size * 2);
    return array.size();
}

TEST_CASE("Testing ResizableArray resize")
{
    for (int s = 10; s < 1e8; s *= 10)
        CHECK(checkResize(s) == s * 2);
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

    for (int i = 0; i < newArray.size(); ++i)
        if (newArray[i] != array[i])
            return -2;

    // Copy operator
    newArray = array;
    if (array.size() != newArray.size())
        return -3;

    for (int i = 0; i < newArray.size(); ++i)
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
