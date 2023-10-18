#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <doctest.h>

#include "./core/base_object.h"

using namespace std::chrono;
using namespace Splash;

namespace BaseObjectTests
{
/*************/
class BaseObjectMock : public BaseObject
{
  public:
    BaseObjectMock()
        : BaseObject()
    {
        registerAttributes();
    }

    void removeAttributeProxy(const std::string& name) { removeAttribute(name); }

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
    std::string _string{""};

    void registerAttributes()
    {
        addAttribute(
            "integer",
            [&](const Values& args) {
                _integer = args[0].as<int>();
                return true;
            },
            [&]() -> Values { return {_integer}; },
            {'i'});
        setAttributeDescription("integer", "An integer attribute");

        addAttribute(
            "float",
            [&](const Values& args) {
                _float = args[0].as<float>();
                return true;
            },
            [&]() -> Values { return {_float}; },
            {'r'});
        setAttributeDescription("float", "A float attribute");
        setAttributeSyncMethod("float", Attribute::Sync::force_async);

        addAttribute(
            "string",
            [&](const Values& args) {
                _string = args[0].as<std::string>();
                return true;
            },
            [&]() -> Values { return {_string}; },
            {'s'});
        setAttributeDescription("string", "A string attribute");
        setAttributeSyncMethod("string", Attribute::Sync::force_sync);

        addAttribute("noGetterAttrib",
            [&](const Values& args) {
                _integer = 0;
                _float = 0.f;
                _string = "zero";
                return true;
            },
            {});

        addAttribute("noSetterAttrib", [&]() -> Values { return {"A getter but no setter"}; });
    }
};
} // namespace BaseObjectTests

/*************/
TEST_CASE("Testing BaseObject class")
{
    auto object = std::make_unique<BaseObjectTests::BaseObjectMock>();
    object->setName("Classe");
    CHECK_EQ(object->getName(), "Classe");

    int integer_value = 42;
    float float_value = 3.1415f;
    std::string string_value = "T'es sûr qu'on dit pas ouiche ?";
    Values array_value{1, 4.2, "sheraf"};

    CHECK(object->setAttribute("integer", {integer_value}) == BaseObject::SetAttrStatus::success);
    CHECK(object->setAttribute("float", {float_value}) == BaseObject::SetAttrStatus::success);
    CHECK(object->setAttribute("string", {string_value}) == BaseObject::SetAttrStatus::success);

    CHECK(object->setAttribute("non_existing_attr", {"whichever value"}) == BaseObject::SetAttrStatus::failure);

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
    CHECK_EQ(value[0].as<std::string>(), string_value);
    CHECK_EQ(object->getAttribute("string").value()[0].as<std::string>(), string_value);

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
        auto name = entry.as<Values>()[0].as<std::string>();
        auto description = entry.as<Values>()[1].as<std::string>();
        CHECK(object->hasAttribute(name));
        CHECK_EQ(description, object->getAttributeDescription(name));
    }

    object->removeAttributeProxy("float");
    CHECK_FALSE(object->hasAttribute("float"));

    CHECK(object->setAttribute("noGetterAttrib", {}) == BaseObject::SetAttrStatus::success);
    CHECK(object->setAttribute("noSetterAttrib", {'b'}) == BaseObject::SetAttrStatus::no_setter);
}

/*************/
TEST_CASE("Testing BaseObject attribute registering")
{
    auto object = std::make_shared<BaseObjectTests::BaseObjectMock>();
    auto someString = std::string("What are you waiting for? Christmas?");
    auto otherString = std::string("Show me the money!");

    CHECK(object->setAttribute("integer", {42}) == BaseObject::SetAttrStatus::success);
    auto handle = object->registerCallback("integer", [&](const std::string& obj, const std::string& attr) { someString = otherString; });
    CHECK(static_cast<bool>(handle));
    object->setAttribute("integer", {1337});
    CHECK(someString == otherString);

    object->unregisterCallback(handle);
    otherString = "I've got a flying machine!";
    object->setAttribute("integer", {42});
    CHECK(someString != otherString);

    object->registerCallback("integer", [&](const std::string& obj, const std::string& attr) { someString = otherString; });
    object->setAttribute("integer", {1337});
    CHECK(someString != otherString);
}

/*************/
TEST_CASE("Testing BaseObject task and periodic task")
{
    auto object = std::make_shared<BaseObjectTests::BaseObjectMock>();
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
    CHECK_NE(object->getAttribute("string").value()[0].as<std::string>(), "Yo dawg!");

    object->setupAsyncTasks();
    std::this_thread::sleep_for(100ms);
    object->runTasks();
    CHECK_EQ(object->getAttribute("string").value()[0].as<std::string>(), "Async task finished!");
}
