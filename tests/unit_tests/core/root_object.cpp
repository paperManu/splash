#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

#include <doctest.h>

#include "./core/graph_object.h"
#include "./core/root_object.h"
#include "./core/serialized_object.h"
#include "./core/value.h"
#include "./image/image.h"

using namespace Splash;

/*************/
class RootObjectMock : public RootObject
{
  public:
    Values _testValue{};

  public:
    RootObjectMock()
        : RootObject()
    {
        registerAttributes();
        _link = std::make_unique<Link>(this, "mock", Link::ChannelType::zmq);
        _name = "world";
        _tree.setName(_name);
    }

    void step()
    {
        _tree.processQueue(true);
        executeTreeCommands();
        runTasks();
        updateTreeFromObjects();
        propagateTree();
        waitSignalBufferObjectUpdated(100);
    }

  private:
    void registerAttributes()
    {
        addAttribute(
            "testValue",
            [&](const Values& args) {
                _testValue = args;
                return true;
            },
            [&]() -> Values { return _testValue; },
            {});
    }
};

/*************/
TEST_CASE("Testing RootObject construction")
{
    auto root = RootObjectMock();
    root.step();
    auto tree = root.getTree();

    CHECK(tree->hasBranchAt("/world"));
    CHECK(tree->hasBranchAt("/world/attributes"));
    CHECK(tree->hasBranchAt("/world/commands"));
    CHECK(tree->hasBranchAt("/world/durations"));
    CHECK(tree->hasBranchAt("/world/logs"));
    CHECK(tree->hasBranchAt("/world/objects"));

    auto attributeList = root.getAttributesList();
    CHECK(std::find(attributeList.cbegin(), attributeList.cend(), "answerMessage") != attributeList.end());
}

/*************/
TEST_CASE("Testing RootObject basic configuration")
{
    auto root = RootObjectMock();

    CHECK_EQ(root.getSocketPrefix(), "");
    CHECK_EQ(root.getConfigurationPath(), "");
    CHECK_EQ(root.getMediaPath(), "");
}

/*************/
TEST_CASE("Testing RootObject object creation and lifetime handling")
{
    std::string objectName = "name";
    auto root = RootObjectMock();
    auto graphObject = root.createObject("image", objectName);
    CHECK(!graphObject.expired());

    auto otherObject = root.createObject("image", objectName);
    CHECK_EQ(graphObject.lock(), otherObject.lock());

    otherObject = root.createObject("mesh", objectName);
    CHECK(otherObject.expired());

    otherObject = root.createObject("nonExistingType", "someOtherName");
    CHECK(otherObject.expired());

    root.disposeObject(objectName);
    root.step();
    CHECK(graphObject.expired());
}

/*************/
TEST_CASE("Testing RootObject attribute set")
{
    auto root = RootObjectMock();

    auto value = Values({1, "one"});
    root.set("world", "testValue", value);
    root.step();
    CHECK_EQ(root._testValue, value);

    auto originalName = "image";
    auto image = root.createObject("image", originalName).lock();

    auto newAlias = "newAlias";
    root.set(originalName, "alias", {newAlias});
    root.step();
    CHECK_EQ(image->getAlias(), newAlias);

    auto otherAlias = "otherAlias";
    root.set(originalName, "alias", {otherAlias}, false);
    CHECK_EQ(image->getAlias(), otherAlias);
}

/*************/
TEST_CASE("Testing RootObject serialized object set")
{
    auto root = RootObjectMock();
    auto imageName = "image";
    auto image = std::dynamic_pointer_cast<Image>(root.createObject("image", imageName).lock());

    auto timestamp = image->getTimestamp();
    auto result = root.setFromSerializedObject(imageName, SerializedObject());
    image->update();
    CHECK_EQ(result, true);
    CHECK_EQ(timestamp, image->getTimestamp());

    auto otherName = "otherImage";
    auto otherImage = std::dynamic_pointer_cast<Image>(root.createObject("image", otherName).lock());
    otherImage->set(512, 512, 3, ImageBufferSpec::Type::UINT8);
    otherImage->update();
    result = root.setFromSerializedObject(imageName, otherImage->serialize());
    while (image->hasSerializedObjectWaiting())
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    image->update();
    CHECK_EQ(result, true);
    CHECK_NE(timestamp, image->getTimestamp());

    result = root.setFromSerializedObject("nonExistingObject", SerializedObject());
    CHECK_EQ(result, false);

    auto blenderName = "geomName";
    root.createObject("blender", blenderName);
    result = root.setFromSerializedObject(blenderName, SerializedObject());
    CHECK_EQ(result, false);
}
