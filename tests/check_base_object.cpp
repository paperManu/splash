#include <memory>
#include <string>

#include <doctest.h>

#include "./core/graph_object.h"
#include "./splash.h"

using namespace std;
using namespace Splash;

/*************/
class GraphObjectMock : public GraphObject
{
  public:
    GraphObjectMock(RootObject* root)
        : GraphObject(root)
    {
        registerAttributes();
    }

  private:
    int _integer{0};
    float _float{0.f};
    string _string{""};

    void registerAttributes()
    {
        addAttribute("integer",
            [&](const Values& args) {
                _integer = args[0].as<int>();
                return true;
            },
            [&]() -> Values { return {_integer}; },
            {'n'});

        addAttribute("float",
            [&](const Values& args) {
                _float = args[0].as<float>();
                return true;
            },
            [&]() -> Values { return {_float}; },
            {'n'});

        addAttribute("string",
            [&](const Values& args) {
                _string = args[0].as<string>();
                return true;
            },
            [&]() -> Values { return {_string}; },
            {'s'});
    }
};

/*************/
TEST_CASE("Testing GraphObject class")
{
    auto object = make_unique<GraphObjectMock>(nullptr);

    int integer_value = 42;
    float float_value = 3.1415;
    string string_value = "T'es sûr qu'on dit pas ouiche ?";
    Values array_value{1, 4.2, "sheraf"};

    object->setAttribute("integer", {integer_value});
    object->setAttribute("float", {float_value});
    object->setAttribute("string", {string_value});
    object->setAttribute("newAttribute", array_value);

    Values value;
    CHECK(object->getAttribute("integer", value) == true);
    CHECK(!value.empty());
    CHECK(value[0].as<int>() == 42);

    CHECK(object->getAttribute("float", value) == true);
    CHECK(!value.empty());
    CHECK(value[0].as<float>() == float_value);

    CHECK(object->getAttribute("string", value) == true);
    CHECK(!value.empty());
    CHECK(value[0].as<string>() == string_value);

    CHECK(object->getAttribute("newAttribute", value) == true);
    CHECK(!value.empty());
    CHECK(value == array_value);

    CHECK(object->getAttribute("inexistingAttribute", value) == false);
    CHECK(value.empty());
}

/*************/
TEST_CASE("Testing GraphObject attribute registering")
{
    auto object = make_shared<GraphObjectMock>(nullptr);
    auto someString = string("What are you waiting for? Christmas?");
    auto otherString = string("Show me the money!");

    object->setAttribute("someAttribute", {42});
    auto handle = object->registerCallback("someAttribute", [&](const string& obj, const string& attr) { someString = otherString; });
    CHECK(static_cast<bool>(handle));
    object->setAttribute("someAttribute", {1337});
    CHECK(someString == otherString);

    object->unregisterCallback(handle);
    otherString = "I've got a flying machine!";
    object->setAttribute("someAttribute", {42});
    CHECK(someString != otherString);

    object->registerCallback("someAttribute", [&](const string& obj, const string& attr) { someString = otherString; });
    object->setAttribute("someAttribute", {1337});
    CHECK(someString != otherString);
}
