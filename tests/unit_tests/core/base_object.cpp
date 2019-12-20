#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <doctest.h>

#include "./core/base_object.h"
#include "./splash.h"

using namespace std;
using namespace Splash;

/*************/
class BaseObjectMock : public BaseObject
{
  public:
    BaseObjectMock()
        : BaseObject()
    {
        registerAttributes();
    }

    void removeAttributeProxy(const string& name) { removeAttribute(name); }

    void setupTasks()
    {
        addTask([this]() -> void { _float = 128.f; });

        addPeriodicTask("counterTask", [this]() -> void { ++_integer; });
        addPeriodicTask("counterTask", [this]() -> void { _integer += 2; });
    }

    void cleanPeriodicTask() { removePeriodicTask("counterTask"); }

    void setupImbricatedPeriodicTasks()
    {
        addPeriodicTask("periodicTask", [this]() -> void {
            addPeriodicTask("periodicerTaks", [this]() -> void {
                _string = "Yo dawg!";
                return;
            });
        });
    }

    void setupAsyncTasks()
    {
        runAsyncTask([this]() -> void { _string = "Async task finished!"; });
    }

  private:
    int _integer{0};
    float _float{0.f};
    string _string{""};

    void registerAttributes()
    {
        addAttribute(
            "integer",
            [&](const Values& args) {
                _integer = args[0].as<int>();
                return true;
            },
            [&]() -> Values { return {_integer}; },
            {'n'});
        setAttributeDescription("integer", "An integer attribute");

        addAttribute(
            "float",
            [&](const Values& args) {
                _float = args[0].as<float>();
                return true;
            },
            [&]() -> Values { return {_float}; },
            {'n'});
        setAttributeDescription("float", "A float attribute");
        setAttributeSyncMethod("float", Attribute::Sync::force_async);

        addAttribute(
            "string",
            [&](const Values& args) {
                _string = args[0].as<string>();
                return true;
            },
            [&]() -> Values { return {_string}; },
            {'s'});
        setAttributeDescription("string", "A string attribute");
        setAttributeSyncMethod("string", Attribute::Sync::force_sync);

        addAttribute("noGetterAttrib", [&](const Values& args) {
            _integer = 0;
            _float = 0.f;
            _string = "zero";
            return true;
        });
    }
};

/*************/
TEST_CASE("Testing BaseObject class")
{
    auto object = make_unique<BaseObjectMock>();
    object->setName("Classe");
    CHECK_EQ(object->getName(), "Classe");

    int integer_value = 42;
    float float_value = 3.1415;
    string string_value = "T'es sûr qu'on dit pas ouiche ?";
    Values array_value{1, 4.2, "sheraf"};

    object->setAttribute("integer", {integer_value});
    object->setAttribute("float", {float_value});
    object->setAttribute("string", {string_value});
    object->setAttribute("newAttribute", array_value);

    Values value;
    CHECK(object->getAttribute("integer", value));
    CHECK(!value.empty());
    CHECK_EQ(value[0].as<int>(), integer_value);
    CHECK_EQ(object->getAttribute("integer").value()[0].as<int>(), integer_value);

    CHECK(object->getAttribute("float", value));
    CHECK(!value.empty());
    CHECK_EQ(value[0].as<float>(), float_value);
    CHECK_EQ(object->getAttribute("float").value()[0].as<float>(), float_value);

    CHECK(object->getAttribute("string", value));
    CHECK(!value.empty());
    CHECK_EQ(value[0].as<string>(), string_value);
    CHECK_EQ(object->getAttribute("string").value()[0].as<string>(), string_value);

    CHECK(object->getAttribute("newAttribute", value) == true);
    CHECK(!value.empty());
    CHECK(value == array_value);
    CHECK_EQ(object->getAttribute("newAttribute").value(), array_value);

    CHECK(object->getAttribute("inexistingAttribute", value) == false);
    CHECK(value.empty());

    CHECK_EQ(object->getAttributeDescription("integer"), "An integer attribute");
    CHECK_EQ(object->getAttributeDescription("float"), "A float attribute");
    CHECK_EQ(object->getAttributeDescription("string"), "A string attribute");

    CHECK_EQ(object->getAttributeSyncMethod("integer"), Attribute::Sync::auto_sync);
    CHECK_EQ(object->getAttributeSyncMethod("float"), Attribute::Sync::force_async);
    CHECK_EQ(object->getAttributeSyncMethod("string"), Attribute::Sync::force_sync);

    auto descriptions = object->getAttributesDescriptions();
    auto attributes = object->getAttributesList();
    for (const auto& entry : descriptions)
    {
        CHECK_EQ(entry.as<Values>().size(), 3);
        auto name = entry.as<Values>()[0].as<string>();
        auto description = entry.as<Values>()[1].as<string>();
        CHECK(object->hasAttribute(name));
        CHECK_EQ(description, object->getAttributeDescription(name));
    }

    object->removeAttributeProxy("float");
    CHECK_FALSE(object->hasAttribute("float"));
}

/*************/
TEST_CASE("Testing BaseObject attribute registering")
{
    auto object = make_shared<BaseObjectMock>();
    auto someString = string("What are you waiting for? Christmas?");
    auto otherString = string("Show me the money!");

    CHECK(object->setAttribute("someAttribute", {42}));
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

/*************/
TEST_CASE("Testing BaseObject task and periodic task")
{
    auto object = make_shared<BaseObjectMock>();
    object->setupTasks();
    object->setAttribute("integer", {0});

    object->runTasks();
    CHECK_EQ(object->getAttribute("integer").value()[0].as<int>(), 2);
    CHECK_EQ(object->getAttribute("float").value()[0].as<float>(), 128.f);

    object->runTasks();
    CHECK_EQ(object->getAttribute("integer").value()[0].as<int>(), 4);
    CHECK_EQ(object->getAttribute("float").value()[0].as<float>(), 128.f);

    object->cleanPeriodicTask();
    object->runTasks();
    CHECK_EQ(object->getAttribute("integer").value()[0].as<int>(), 4);

    object->setupImbricatedPeriodicTasks();
    object->runTasks();
    object->runTasks();
    CHECK_NE(object->getAttribute("string").value()[0].as<string>(), "Yo dawg!");

    object->setupAsyncTasks();
    this_thread::sleep_for(100ms);
    object->runTasks();
    CHECK_EQ(object->getAttribute("string").value()[0].as<string>(), "Async task finished!");
}
