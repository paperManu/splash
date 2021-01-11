#include <doctest.h>

#include "./core/attribute.h"

using namespace Splash;

/*************/
TEST_CASE("Testing Attribute usage")
{
    int value = 0;
    auto attr = Attribute("attribute",
        [&](const Values& args) {
            value = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {value}; },
        {'i'});
    attr.setDescription("A very simple attribute");
    CHECK_EQ(attr.getDescription(), "A very simple attribute");
    CHECK_EQ(attr.getSyncMethod(), Attribute::Sync::auto_sync);

    attr.setSyncMethod(Attribute::Sync::force_sync);
    CHECK_EQ(attr.getSyncMethod(), Attribute::Sync::force_sync);

    CHECK(attr.hasGetter());

    CHECK(attr({42}));
    CHECK_EQ(attr()[0].as<int>(), 42);
    CHECK_FALSE(attr({"Patate"}));
    CHECK(attr({42, "Patate"}));

    CHECK(attr({42}));
    attr.lock();
    CHECK(attr.isLocked());
    CHECK_FALSE(attr({512}));
    CHECK_EQ(attr()[0].as<int>(), 42);
    attr.unlock();

    attr = Attribute(
        "attribute",
        [&](const Values& args) {
            value = args[0].as<int>();
            return true;
        },
        [&]() -> Values {
            return {value, "some string"};
        },
        {'i', 's'});
    CHECK_FALSE(attr({3.14159}));

    attr = Attribute("attribute",
        [&](const Values& args) {
            value = args[0].as<int>();
            return true;
        },
        nullptr,
        {'s'});
    CHECK(attr({"A girl has no name"}));
    CHECK(attr().empty());
}
