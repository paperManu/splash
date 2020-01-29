#include <doctest.h>

#include "./core/name_registry.h"

using namespace Splash;

/*************/
TEST_CASE("Testing NameRegistry")
{
    auto registry = NameRegistry();
    auto prefix = "TEST";
    auto name = registry.generateName(prefix);
    CHECK(name.find(prefix) != std::string::npos);

    auto newName = registry.generateName(prefix);
    CHECK_NE(name, newName);

    auto someName = "quiddity";
    CHECK(registry.registerName(someName));
    CHECK(!registry.registerName(someName));

    registry.unregisterName(someName);
    CHECK(registry.registerName(someName));
}
