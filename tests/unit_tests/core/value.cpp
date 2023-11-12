#include <doctest.h>
#include <iostream>
#include <random>
#include <vector>

#include "./core/serialize/serialize_value.h"
#include "./core/serializer.h"
#include "./core/value.h"

using namespace Splash;

/*************/
TEST_CASE("Testing type change")
{
    SUBCASE("Converting from a bool")
    {
        auto value = Value(true);
        CHECK(value.as<bool>() == true);
        CHECK(value.as<std::string>() == "true");
    }

    SUBCASE("Converting from a float")
    {
        auto value = Value(5.f);
        CHECK(value.as<int>() == 5);
        CHECK(value.as<long>() == 5);
        CHECK(value.as<float>() == 5.f);
        CHECK(value.as<std::string>().find("5.0") == 0);
    }

    SUBCASE("Converting from a integer")
    {
        auto value = Value(5);
        CHECK(value.as<int>() == 5);
        CHECK(value.as<long>() == 5);
        CHECK(value.as<float>() == 5.f);
        CHECK(value.as<std::string>().find("5") == 0);
    }

    SUBCASE("Converting from a string")
    {
        auto value = Value("15");
        CHECK(value.as<int>() == 15);
        CHECK(value.as<long>() == 15);
        CHECK(value.as<float>() == 15.f);
        CHECK(value.as<std::string>() == "15");
    }
}

/*************/
bool compareIntToValues(const std::vector<int>& a, const Values& b)
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
    auto buffer = std::vector<int>((size_t)128);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 512);
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

/*************/
TEST_CASE("Testing buffer in Value")
{
    Value::Buffer buffer(256);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    for (uint32_t i = 0; i < buffer.size(); ++i)
        buffer[i] = dist(gen);

    Value value(buffer);

    bool isEqual = true;
    for (uint32_t i = 0; i < buffer.size(); ++i)
        isEqual &= (buffer[i] == value.as<Value::Buffer>()[i]);
    CHECK(isEqual);

    auto otherValue = value;
    CHECK(otherValue == value);

    isEqual = true;
    for (uint32_t i = 0; i < buffer.size(); ++i)
        isEqual &= (buffer[i] == otherValue.as<Value::Buffer>()[i]);
    CHECK(isEqual);
}

/*************/
TEST_CASE("Testing Value serialization")
{
    std::string testString("One to rule them all");

    CHECK(Serial::getSize(Value(true)) == sizeof(Value::Type) + sizeof(bool));
    CHECK(Serial::getSize(Value(42)) == sizeof(Value::Type) + sizeof(int64_t));
    CHECK(Serial::getSize(Value(2.71828f)) == sizeof(Value::Type) + sizeof(double));
    CHECK(Serial::getSize(Value(testString)) == sizeof(Value::Type) + sizeof(uint32_t) + testString.size() * sizeof(char));
    CHECK(Serial::getSize(Value(Values({3.14159f, 42, testString}))) ==
          sizeof(Value::Type) + sizeof(uint32_t) + sizeof(Value::Type) * 3 + sizeof(double) + sizeof(int64_t) + sizeof(uint32_t) + sizeof(char) * testString.size());

    {
        std::vector<uint8_t> buffer;
        Serial::serialize(Value(42), buffer);
        auto bufferPtr = buffer.data();
        CHECK(*reinterpret_cast<Value::Type*>(bufferPtr) == Value::Type::integer);
        bufferPtr += sizeof(Value::Type);
        CHECK(*reinterpret_cast<int64_t*>(bufferPtr) == 42);
    }

    {
        std::vector<uint8_t> buffer;
        Serial::serialize(Value(3.14159), buffer);
        auto bufferPtr = buffer.data();
        CHECK(*reinterpret_cast<Value::Type*>(bufferPtr) == Value::Type::real);
        bufferPtr += sizeof(Value::Type);
        CHECK(*reinterpret_cast<double*>(bufferPtr) == 3.14159);
    }

    {
        std::vector<uint8_t> buffer;
        Serial::serialize(Value(testString), buffer);
        auto bufferPtr = buffer.data();
        CHECK(*reinterpret_cast<Value::Type*>(bufferPtr) == Value::Type::string);
        bufferPtr += sizeof(Value::Type);
        auto stringLength = *reinterpret_cast<uint32_t*>(bufferPtr);
        CHECK(stringLength == testString.size());
        bufferPtr += sizeof(uint32_t);
        CHECK(std::string(reinterpret_cast<char*>(bufferPtr), stringLength) == testString);
    }

    {
        std::vector<uint8_t> buffer;
        auto data = Value(Values({42, 3.14159}));
        Serial::serialize(data, buffer);
        auto bufferPtr = buffer.data();
        CHECK(*reinterpret_cast<Value::Type*>(bufferPtr) == Value::Type::values);
        bufferPtr += sizeof(Value::Type);
        CHECK(*reinterpret_cast<uint32_t*>(bufferPtr) == data.size());
        bufferPtr += sizeof(uint32_t);
        CHECK(*reinterpret_cast<Value::Type*>(bufferPtr) == Value::Type::integer);
        bufferPtr += sizeof(Value::Type);
        CHECK(*reinterpret_cast<int64_t*>(bufferPtr) == data.as<Values>()[0].as<int64_t>());
        bufferPtr += sizeof(int64_t);
        CHECK(*reinterpret_cast<Value::Type*>(bufferPtr) == Value::Type::real);
        bufferPtr += sizeof(Value::Type);
        CHECK(*reinterpret_cast<double*>(bufferPtr) == data.as<Values>()[1].as<double>());
    }

    {
        std::vector<uint8_t> buffer;
        auto data = Value(false);
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<Value>(buffer);
        CHECK(data == outData);
    }

    {
        std::vector<uint8_t> buffer;
        auto data = Value(42);
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<Value>(buffer);
        CHECK(data == outData);
    }

    {
        std::vector<uint8_t> buffer;
        auto data = Value(3.14159);
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<Value>(buffer);
        CHECK(data == outData);
    }

    {
        std::vector<uint8_t> buffer;
        auto data = Value(testString);
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<Value>(buffer);
        CHECK(data == outData);
    }

    {
        std::vector<uint8_t> buffer;
        auto data = Value(Values({42, 2.71828, testString}));
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<Value>(buffer);
        CHECK(data == outData);
    }

    {
        Value::Buffer inputBuffer(256);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (uint32_t i = 0; i < inputBuffer.size(); ++i)
            inputBuffer[i] = dist(gen);

        std::vector<uint8_t> buffer;
        auto data = Value(inputBuffer);
        Serial::serialize(data, buffer);
        auto outData = Serial::deserialize<Value>(buffer);
        CHECK(data == outData);
    }
}
