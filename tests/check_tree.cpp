#include <doctest.h>

#include <chrono>
#include <utility>

#include "./core/tree.h"
#include "./utils/log.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing the basic functionnalities of the Tree")
{
    Log::get().setVerbosity(Log::ERROR);

    Tree::Root tree;
    CHECK(tree.addBranchAt("/some_object") == true);
    CHECK(tree.addBranchAt("/some_object/some_other_object") == true);
    CHECK(tree.addBranchAt("/some_object/some_other_object") == false);
    CHECK(tree.removeBranchAt("/some_object") == true);
    CHECK(tree.removeBranchAt("/some_object") == false);

    CHECK(tree.addBranchAt("/some_object") == true);
    CHECK(tree.addLeafAt("/some_object/a_leaf") == true);
    CHECK(tree.addLeafAt("/some_object/a_leaf") == false);

    auto value = Values({1.0, "I've got a flying machine", false});
    CHECK(tree.addLeafAt("/some_object/another_leaf", {1.0, "I've got a flying machine", false}) == true);

    Value leafValue;
    tree.getValueForLeafAt("/some_object/another_leaf", leafValue);
    CHECK(leafValue == value);

    value = Values({"No you don't"});
    CHECK(tree.setValueForLeafAt("/some_object/another_leaf", value, chrono::system_clock::now()));
    tree.getValueForLeafAt("/some_object/another_leaf", leafValue);
    CHECK(leafValue == value);

    CHECK(tree.removeLeafAt("/some_object/a_leaf") == true);
    CHECK(tree.removeLeafAt("/some_object/a_leaf") == false);
}

/*************/
TEST_CASE("Testing the Seed queue")
{
    Tree::Root tree;
    tree.addSeedToQueue(Tree::Task::AddBranch, Values({"/some_object"}));
    tree.addSeedToQueue(Tree::Task::AddLeaf, Values({"/some_object/a_leaf"}));

    CHECK_NOTHROW(tree.processQueue());
    CHECK(tree.addBranchAt("/some_object") == false);
    CHECK(tree.addLeafAt("/some_object/a_leaf") == false);

    tree.addSeedToQueue(Tree::Task::RemoveLeaf, Values({"/some_object/a_leaf"}));
    tree.addSeedToQueue(Tree::Task::RemoveBranch, Values({"/some_object"}));

    CHECK_NOTHROW(tree.processQueue());
    CHECK(tree.addBranchAt("/some_object") == true);
    CHECK(tree.addLeafAt("/some_object/a_leaf") == true);

    auto value = Values({1.0, "I've got a flying machine", false});
    tree.addSeedToQueue(Tree::Task::SetLeaf, Values({"/some_object/a_leaf", value}));

    CHECK_NOTHROW(tree.processQueue());
    Value leafValue;
    CHECK(tree.getValueForLeafAt("/some_object/a_leaf", leafValue) == true);
    CHECK(leafValue == value);

    tree.addSeedToQueue(Tree::Task::RemoveLeaf, Values({"/some_object/a_leaf"}));
    tree.addSeedToQueue(Tree::Task::RemoveBranch, Values({"/some_object"}));

    CHECK_NOTHROW(tree.processQueue());
}

/*************/
TEST_CASE("Testing the synchronization between trees")
{
    string error{};
    Tree::Root maple, oak;
    auto value = Values({1.0, "I've got a flying machine", false});

    maple.addBranchAt("/some_branch");
    maple.addLeafAt("/some_branch/some_leaf", value);
    auto updates = maple.getSeedList();

    oak.addSeedsToQueue(updates);
    CHECK_NOTHROW(oak.processQueue());
    CHECK(maple == oak);

    CHECK(oak.addBranchAt("/some_branch") == false);
    CHECK(oak.addLeafAt("/some_branch/some_leaf") == false);
    CHECK(oak.getError(error));
    CHECK(!error.empty());

    maple.removeLeafAt("/some_branch/some_leaf");
    maple.removeBranchAt("/some_branch");

    updates = maple.getSeedList();
    oak.addSeedsToQueue(updates);
    CHECK_NOTHROW(oak.processQueue());
    CHECK(!oak.getError(error));
    CHECK(error.empty());

    CHECK(maple == oak);
}

/*************/
TEST_CASE("Testing the chronology handling of updates")
{
    Tree::Root maple, oak, beech;

    maple.addBranchAt("/a_branch");
    oak.addBranchAt("/a_branch");
    maple.addLeafAt("/a_branch/a_leaf");
    oak.addLeafAt("/a_branch/a_leaf");
    oak.setValueForLeafAt("/a_branch/a_leaf", {"Fresh meat!"});
    maple.setValueForLeafAt("/a_branch/a_leaf", {"Stop clicking on me!"});

    beech.addSeedsToQueue(maple.getSeedList());
    beech.addSeedsToQueue(oak.getSeedList());

    Value leafValue;
    CHECK_NOTHROW(beech.processQueue());
    CHECK(beech.getValueForLeafAt("/a_branch/a_leaf", leafValue) == true);
    CHECK(leafValue == Values({"Stop clicking on me!"}));
    CHECK(beech.hasError());
}

/*************/
TEST_CASE("Testing the Leaf's callbacks")
{
    Tree::Root maple;
    maple.addLeafAt("/a_leaf");
    auto leaf = maple.getLeafAt("/a_leaf");
    CHECK(leaf != nullptr);

    Value extValue{""};
    auto callbackID = leaf->addCallback([&extValue](Value value, chrono::system_clock::time_point timestamp) { extValue = value; });
    maple.setValueForLeafAt("/a_leaf", {"Ceci n'est pas un test"});

    CHECK(extValue == Values({"Ceci n'est pas un test"}));
    CHECK(leaf->removeCallback(callbackID));

    maple.setValueForLeafAt("/a_leaf", {"Ceci non plus"});
    CHECK(extValue == Values({"Ceci n'est pas un test"}));
}

/*************/
TEST_CASE("Testing propagation through a main tree")
{
    Tree::Root main, maple, oak;
    maple.addLeafAt("/some_leaf");
    maple.addBranchAt("/a_branch");
    auto seeds = maple.getSeedList();
    main.addSeedsToQueue(seeds);
    main.processQueue(true);
    seeds = main.getSeedList();
    oak.addSeedsToQueue(seeds);
    oak.processQueue();

    CHECK(main == maple);
    CHECK(main == oak);
}
