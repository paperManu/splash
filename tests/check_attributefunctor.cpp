#include <doctest.h>

#include "./splash.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing Attribute")
{
    int value = 0;
    auto attr = Attribute("attribute",
        [&](const Values& args) {
            value = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {value}; },
        {'n'});

    CHECK(attr({42}) == true);
    CHECK(attr()[0].as<int>() == 42);
    CHECK(attr({"Patate"}) == false);
    CHECK(attr({42, "Patate"}) == true);

    attr({42});
    attr.lock();
    attr({512});
    CHECK(attr()[0].as<int>() == 42);
    attr.unlock();
}
