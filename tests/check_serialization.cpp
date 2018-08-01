#include <doctest.h>

#include <chrono>
#include <deque>
#include <tuple>
#include <vector>

#include "./core/serializer.h"
#include "./utils/log.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing serialized size computation")
{
    CHECK(Serial::getSize((int)18) == sizeof(int));
    CHECK(Serial::getSize((float)18.f) == sizeof(float));
    CHECK(Serial::getSize((double)18.0) == sizeof(double));

    vector<int> vectorOfInts{1, 2, 3, 4};
    CHECK(Serial::getSize(vectorOfInts) == vectorOfInts.size() * sizeof(int) + sizeof(size_t));

    deque<float> dequeOfFloat{3.14159f, 42.f, 2.71828f};
    CHECK(Serial::getSize(dequeOfFloat) == dequeOfFloat.size() * sizeof(float) + sizeof(size_t));

    std::string someText{"So long, and thanks for the fish!"};
    CHECK(Serial::getSize(someText) == someText.size() * sizeof(char) + sizeof(size_t));

    CHECK(Serial::getSize(chrono::system_clock::time_point(chrono::duration<int64_t, std::milli>(123456))) == sizeof(int64_t));
}

/*************/
TEST_CASE("Testing serialized size computation for containers of containers")
{
    {
        vector<vector<int>> data;
        for (uint32_t i = 0; i < 4; ++i)
            data.push_back(vector<int>({2, 4, 8, 16}));

        CHECK(Serial::getSize(data) == sizeof(size_t) + data.size() * (sizeof(size_t) + data[0].size() * sizeof(int)));
    }

    {
        deque<deque<int>> data;
        for (uint32_t i = 0; i < 4; ++i)
            data.push_back(deque<int>({2, 4, 8, 16}));

        CHECK(Serial::getSize(data) == sizeof(size_t) + data.size() * (sizeof(size_t) + data[0].size() * sizeof(int)));
    }
}

/*************/
TEST_CASE("Testing serialized size computation for tuples")
{
    auto testString = string("Show me the money");
    auto data = make_tuple(3.1415f, 42, testString);
    CHECK(Serial::getSize(data) == sizeof(float) + sizeof(int) + sizeof(size_t) + testString.size());
}

/*************/
TEST_CASE("Testing elementary serialization")
{
    string testString("Fresh meat");
    vector<uint8_t> buffer;
    Serial::serialize((int)7243, buffer);
    Serial::serialize((float)3.14159f, buffer);
    Serial::serialize((double)2.71828, buffer);
    Serial::serialize(testString, buffer);
    Serial::serialize(chrono::system_clock::time_point(chrono::duration<int64_t, std::milli>(123456)), buffer);

    auto bufferPtr = buffer.data();
    CHECK(*reinterpret_cast<int*>(bufferPtr) == 7243);
    bufferPtr += sizeof(int);
    CHECK(*reinterpret_cast<float*>(bufferPtr) == 3.14159f);
    bufferPtr += sizeof(float);
    CHECK(*reinterpret_cast<double*>(bufferPtr) == 2.71828);
    bufferPtr += sizeof(double);
    auto stringLength = *reinterpret_cast<size_t*>(bufferPtr);
    CHECK(stringLength == testString.size());
    bufferPtr += sizeof(size_t);
    CHECK(string(reinterpret_cast<char*>(bufferPtr), stringLength) == testString);
    bufferPtr += sizeof(char) * testString.size();
    CHECK(*reinterpret_cast<int64_t*>(bufferPtr) == 123456);
}

/*************/
TEST_CASE("Testing serialization of iterable containers")
{
    {
        vector<uint8_t> buffer;
        vector<int> data{1, 1, 2, 3, 5, 8};
        Serial::serialize(data, buffer);

        auto bufferPtr = reinterpret_cast<int*>(buffer.data() + sizeof(size_t));
        CHECK(buffer.size() == Serial::getSize(data));
        for (uint32_t i = 0; i < data.size(); ++i)
            CHECK(data[i] == bufferPtr[i]);
    }

    {
        vector<uint8_t> buffer;
        deque<float> data{3.14159f, 2.71828f};
        Serial::serialize(data, buffer);
        auto bufferPtr = reinterpret_cast<float*>(buffer.data() + sizeof(size_t));
        CHECK(buffer.size() == Serial::getSize(data));
        for (uint32_t i = 0; i < data.size(); ++i)
            CHECK(data[i] == bufferPtr[i]);
    }
}

/*************/
TEST_CASE("Testing serialization of tuples")
{
    auto testString = string("Show me the money");
    vector<uint8_t> buffer;
    auto data = make_tuple(3.14159f, 42, testString);
    Serial::serialize(data, buffer);

    auto bufferPtr = buffer.data();
    CHECK(*reinterpret_cast<float*>(bufferPtr) == 3.14159f);
    bufferPtr += sizeof(float);
    CHECK(*reinterpret_cast<int*>(bufferPtr) == 42);
    bufferPtr += sizeof(int);
    auto stringLength = *reinterpret_cast<size_t*>(bufferPtr);
    CHECK(stringLength == testString.size());
    bufferPtr += sizeof(size_t);
    CHECK(string(reinterpret_cast<char*>(bufferPtr), stringLength) == testString);
}

/*************/
TEST_CASE("Testing deserialization")
{
    {
        vector<uint8_t> buffer;
        Serial::serialize((int)42, buffer);
        CHECK(Serial::deserialize<int>(buffer) == 42);
    }

    {
        vector<uint8_t> buffer;
        vector<float> data{3.14159f, 2.71828f};
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<vector<float>>(buffer);
        for (uint32_t i = 0; i < data.size(); ++i)
            CHECK(data[i] == outData[i]);
    }

    {
        vector<uint8_t> buffer;
        deque<int> data{1, 1, 2, 3, 5, 8, 13};
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<deque<int>>(buffer);
        for (uint32_t i = 0; i < data.size(); ++i)
            CHECK(data[i] == outData[i]);
    }

    {
        vector<uint8_t> buffer;
        string data{"This is a good day to die"};
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<string>(buffer);
        CHECK(data == outData);
    }

    {
        vector<uint8_t> buffer;
        auto data = chrono::system_clock::time_point(chrono::duration<int64_t, std::milli>(123456));
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<chrono::system_clock::time_point>(buffer);
        CHECK(data == outData);
    }

    {
        auto testString = string("Show me the money");
        vector<uint8_t> buffer;
        auto data = make_tuple(3.14159f, 42, testString);
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<tuple<float, int, string>>(buffer);
        CHECK(std::get<0>(data) == std::get<0>(outData));
        CHECK(std::get<1>(data) == std::get<1>(outData));
        CHECK(std::get<2>(data) == std::get<2>(outData));
    }
}
