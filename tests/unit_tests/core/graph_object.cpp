#include <memory>

#include <doctest.h>

#include "./core/graph_object.h"
#include "./core/root_object.h"

using namespace Splash;

/*************/
class GraphObjectMock : public GraphObject
{
  public:
    GraphObjectMock(RootObject* root)
        : GraphObject(root)
    {
        _renderingPriority = Priority::NO_RENDER;
    }

    bool isConnectedToRemote() const { return _isConnectedToRemote; }

    bool linkIt(const std::shared_ptr<GraphObject>&) override { return true; }
};

/*************/
TEST_CASE("Testing GraphObject getters and setters")
{
    auto root = RootObject();
    auto object = GraphObjectMock(&root);
    CHECK_EQ(&root, object.getRoot());
    CHECK_EQ(object.getCategory(), GraphObject::Category::MISC);

    auto remoteTypeName = "remoteType";
    object.setRemoteType(remoteTypeName);
    CHECK_EQ(remoteTypeName, object.getRemoteType());
    CHECK(object.isConnectedToRemote());

    CHECK(object.getSavable());
    object.setSavable(false);
    CHECK(!object.getSavable());

    CHECK_EQ(object.getRenderingPriority(), GraphObject::Priority::NO_RENDER);
    object.setRenderingPriority(GraphObject::Priority::CAMERA);
    CHECK_EQ(object.getRenderingPriority(), GraphObject::Priority::CAMERA);
    object.setAttribute("priorityShift", {5});
    CHECK(object.getRenderingPriority() > GraphObject::Priority::CAMERA);
}

/*************/
TEST_CASE("Testing GraphObject linking and unlinking")
{
    auto root = RootObject();
    auto firstObject = std::make_shared<GraphObjectMock>(&root);
    firstObject->setName("firstObject");
    auto secondObject = std::make_shared<GraphObjectMock>(&root);
    secondObject->setName("secondObject");

    firstObject->linkTo(secondObject);
    auto linkedObjects = firstObject->getLinkedObjects();
    auto foundObject = std::find_if(linkedObjects.cbegin(), linkedObjects.cend(), [&](auto linkedObject) -> bool {
        auto object = linkedObject.lock();
        if (!object)
            return false;
        if (object != secondObject)
            return false;
        return true;
    });
    CHECK(foundObject != linkedObjects.end());

    firstObject->unlinkFrom(secondObject);
    linkedObjects = firstObject->getLinkedObjects();
    foundObject = std::find_if(linkedObjects.cbegin(), linkedObjects.cend(), [&](auto linkedObject) -> bool {
        auto object = linkedObject.lock();
        if (!object)
            return false;
        if (object != secondObject)
            return false;
        return true;
    });
    CHECK(foundObject == linkedObjects.end());
}

/*************/
TEST_CASE("Testing GraphObject tree updates")
{
    auto root = RootObject();
    auto object = std::make_shared<GraphObjectMock>(&root);
    object->setName("object");
    {
        auto tree = root.getTree();
        CHECK(tree->hasBranchAt("/" + root.getName() + "/objects/object"));
    }
    object->setName("newName");
    {
        auto tree = root.getTree();
        CHECK(tree->hasBranchAt("/" + root.getName() + "/objects/newName"));
    }
}
