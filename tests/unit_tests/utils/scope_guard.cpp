#include <doctest.h>

#include "./utils/scope_guard.h"

using namespace Splash;

/*************/
TEST_CASE("Testing OnScopeExit")
{
    int value = 42;
    {
        OnScopeExit { value = 16384; };
    }
    CHECK_EQ(value, 16384);
}
