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
        CHECK(value.as<int>() == 5);
        CHECK(value.as<long>() == 5);
        CHECK(value.as<float>() == 5.f);
        CHECK(value.as<string>().find("5.0") == 0);
    }

    SUBCASE("Converting from a integer")
    {
        auto value = Value(5);
        CHECK(value.as<int>() == 5);
        CHECK(value.as<long>() == 5);
        CHECK(value.as<float>() == 5.f);
        CHECK(value.as<string>().find("5") == 0);
    }

    SUBCASE("Converting from a string")
    {
        auto value = Value("15");
        CHECK(value.as<int>() == 15);
        CHECK(value.as<long>() == 15);
        CHECK(value.as<float>() == 15.f);
        CHECK(value.as<string>() == "15");
    }
}

/*************/
bool compareIntToValues(const vector<int>& a, const Values& b)
{
    if (a.size() != b.size())
        return false;

    int index = 0;
    for (const auto& value : b)
    {
        if (a[index] != value.as<int>())
            return false;
        ++index;
    }

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
    CHECK(compareIntToValues(buffer, values.as<Values>()));
    CHECK(values.size() == buffer.size());
}

/*************/
TEST_CASE("Testing Values comparison")
{
    auto values = Value();
    for (int i = 0; i < 128; ++i)
        values.as<Values>().push_back(7);

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
