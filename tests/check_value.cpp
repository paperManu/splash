#include <doctest.h>
#include <random>
#include <vector>

#include "./splash.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing type change")
{
    SUBCASE("Converting from a float")
    {
        auto value = Value(5.f);
        CHECK(value.asInt() == 5);
        CHECK(value.asLong() == 5);
        CHECK(value.asFloat() == 5.f);
        CHECK(value.asString().find("5.0") == 0);
    }

    SUBCASE("Converting from a integer")
    {
        auto value = Value(5);
        CHECK(value.asInt() == 5);
        CHECK(value.asLong() == 5);
        CHECK(value.asFloat() == 5.f);
        CHECK(value.asString().find("5") == 0);
    }

    SUBCASE("Converting from a string")
    {
        auto value = Value("15");
        CHECK(value.asInt() == 15);
        CHECK(value.asLong() == 15);
        CHECK(value.asFloat() == 15.f);
        CHECK(value.asString() == "15");
    }
}

/*************/
bool compareIntToValues(const vector<int>& a, const Values& b)
{
    if (a.size() != b.size())
        return false;

    for (int i = 0; i < a.size(); ++i)
        if (a[i] != b[i].asInt())
            return false;

    return true;
}

TEST_CASE("Testing initialization from iterators")
{
    auto buffer = vector<int>((size_t)128);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, 512);
    for (auto& b : buffer)
        b = dist(gen);

    auto values = Value(buffer.begin(), buffer.end());
    CHECK(compareIntToValues(buffer, values.asValues()));
}

/*************/
TEST_CASE("Testing Values comparison")
{
    auto values = Value();
    for (int i = 0; i < 128; ++i)
        values.asValues().push_back(7);

    auto valueInt = Value(42);
    auto valueFloat = Value(M_PI);
    auto valueString = Value("Le train de tes injures roule sur le rail de mon indifférence");

    CHECK(values == values);
    CHECK(valueInt == 42);
    CHECK(valueFloat == M_PI);
    CHECK(valueString == "Le train de tes injures roule sur le rail de mon indifférence");
    CHECK(values != valueInt);
    CHECK(valueString != valueFloat);
}
