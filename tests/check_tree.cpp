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
    CHECK(tree.createBranchAt("/some_object") == true);
    CHECK(tree.createBranchAt("/some_object/some_other_object") == true);
    CHECK(tree.createBranchAt("/some_object/some_other_object") == false);
    CHECK(tree.removeBranchAt("/some_object") == true);
    CHECK(tree.removeBranchAt("/some_object") == false);

    CHECK(tree.createBranchAt("/some_object") == true);
    CHECK(tree.createLeafAt("/some_object/a_leaf") == true);
    CHECK(tree.createLeafAt("/some_object/a_leaf") == false);

    CHECK(tree.createBranchAt("/some/branch/hidden/in/tree") == true);
    CHECK(tree.hasBranchAt("/some/branch/hidden/in/tree") == true);

    CHECK(tree.createLeafAt("/some/leaf/hidden/in/tree") == true);
    CHECK(tree.hasLeafAt("/some/leaf/hidden/in/tree") == true);

    auto value = Values({1.0, "I've got a flying machine", false});
    CHECK(tree.createLeafAt("/some_object/another_leaf", {1.0, "I've got a flying machine", false}) == true);

    Value leafValue;
    tree.getValueForLeafAt("/some_object/another_leaf", leafValue);
    CHECK(leafValue == value);

    value = {"No you don't"};
    CHECK(tree.setValueForLeafAt("/some_object/another_leaf", value, chrono::system_clock::now()));
    tree.getValueForLeafAt("/some_object/another_leaf", leafValue);
    CHECK(leafValue == value);

    CHECK(tree.removeLeafAt("/some_object/a_leaf") == true);
    CHECK(tree.removeLeafAt("/some_object/a_leaf") == false);

    tree.createBranchAt("/random_branch");
    tree.createBranchAt("/randomer_branch");

    CHECK(tree.renameBranchAt("/random_branch", "randomer_branch") == false);
    CHECK(tree.renameBranchAt("/random_branch", "randomerer_branch") == true);
    CHECK(tree.hasBranchAt("/randomerer_branch") == true);
    CHECK(tree.hasBranchAt("/random_branch") == false);

    tree.createLeafAt("/randomerer_branch/potatoe");
    tree.createLeafAt("/randomerer_branch/salad");

    CHECK(tree.renameLeafAt("/randomerer_branch/potatoe", "salad") == false);
    CHECK(tree.renameLeafAt("/randomerer_branch/potatoe", "burger") == true);
    CHECK(tree.hasLeafAt("/randomerer_branch/burger") == true);
    CHECK(tree.hasLeafAt("/randomerer_branch/potatoe") == false);
}

/*************/
TEST_CASE("Testing the Seed queue")
{
    Tree::Root tree;
    tree.addSeedToQueue(Tree::Task::AddBranch, Values({"/some_object"}));
    tree.addSeedToQueue(Tree::Task::AddLeaf, Values({"/some_object/a_leaf"}));

    CHECK_NOTHROW(tree.processQueue());
    CHECK(tree.createBranchAt("/some_object") == false);
    CHECK(tree.createLeafAt("/some_object/a_leaf") == false);

    tree.addSeedToQueue(Tree::Task::RemoveLeaf, Values({"/some_object/a_leaf"}));
    tree.addSeedToQueue(Tree::Task::RemoveBranch, Values({"/some_object"}));

    CHECK_NOTHROW(tree.processQueue());
    CHECK(tree.createBranchAt("/some_object") == true);
    CHECK(tree.createLeafAt("/some_object/a_leaf") == true);

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

    maple.createBranchAt("/some_branch");
    maple.createLeafAt("/some_branch/some_leaf", value);
    maple.createBranchAt("/some_branch/child_branch");
    maple.renameBranchAt("/some_branch/child_branch", "you_are_my_son");
    auto updates = maple.getSeedList();

    oak.addSeedsToQueue(updates);
    CHECK_NOTHROW(oak.processQueue());
    CHECK(maple == oak);

    CHECK(oak.createBranchAt("/some_branch") == false);
    CHECK(oak.createLeafAt("/some_branch/some_leaf") == false);
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
TEST_CASE("Testing adding and cutting existing branches and leaves")
{
    Tree::Root maple, oak, beech;

    oak.createBranchAt("/a_branch");
    oak.createLeafAt("/a_branch/some_leaf");
    oak.setValueForLeafAt("/a_branch/some_leaf", {"This is not a pie", 3.14159f});
    oak.createLeafAt("/a_leaf");
    oak.setValueForLeafAt("/a_leaf", "Some oak's leaf");

    auto oakSeeds = oak.getSeedList();
    beech.addSeedsToQueue(oakSeeds);
    beech.processQueue();
    CHECK(oak == beech);

    auto branch = oak.cutBranchAt("/a_branch");
    CHECK(branch.get() != nullptr);
    auto leaf = oak.cutLeafAt("/a_leaf");
    CHECK(leaf.get() != nullptr);
    oakSeeds = oak.getSeedList();

    maple.addBranchAt("/", move(branch));
    maple.addLeafAt("/", move(leaf));
    CHECK(maple == beech);
    CHECK(oak != beech);

    auto mapleSeeds = maple.getSeedList();
    oak.addSeedsToQueue(mapleSeeds);
    oak.processQueue();
    CHECK(maple == oak);

    maple.addSeedsToQueue(oakSeeds);
    maple.processQueue();
    CHECK(maple == Tree::Root());
}

/*************/
TEST_CASE("Testing the chronology handling of updates")
{
    Tree::Root maple, oak, beech;

    maple.createBranchAt("/a_branch");
    oak.createBranchAt("/a_branch");
    maple.createLeafAt("/a_branch/a_leaf");
    oak.createLeafAt("/a_branch/a_leaf");
    oak.setValueForLeafAt("/a_branch/a_leaf", "Fresh meat!");
    maple.setValueForLeafAt("/a_branch/a_leaf", "Stop clicking on me!");

    beech.addSeedsToQueue(maple.getSeedList());
    beech.addSeedsToQueue(oak.getSeedList());

    Value leafValue;
    CHECK_NOTHROW(beech.processQueue());
    CHECK(beech.getValueForLeafAt("/a_branch/a_leaf", leafValue) == true);
    CHECK(leafValue == Value("Stop clicking on me!"));
    CHECK(beech.hasError());
}

/*************/
TEST_CASE("Testing the Branch's callbacks")
{
    Tree::Root maple;
    string lastName = "";
    maple.addCallbackToBranchAt("/", Tree::Branch::Task::AddBranch, [&lastName](Tree::Branch& branch, string name) { lastName = name; });
    maple.addCallbackToBranchAt("/", Tree::Branch::Task::RemoveBranch, [&lastName](Tree::Branch& branch, string name) { lastName = name; });
    maple.addCallbackToBranchAt("/", Tree::Branch::Task::AddLeaf, [&lastName](Tree::Branch& branch, string name) { lastName = name; });
    maple.addCallbackToBranchAt("/", Tree::Branch::Task::RemoveLeaf, [&lastName](Tree::Branch& branch, string name) { lastName = name; });

    maple.createBranchAt("/some_branch");
    CHECK(lastName == "some_branch");
    lastName = "";
    maple.removeBranchAt("/some_branch");
    CHECK(lastName == "some_branch");

    maple.createLeafAt("/some_leaf");
    CHECK(lastName == "some_leaf");
    lastName = "";
    maple.removeLeafAt("/some_leaf");
    CHECK(lastName == "some_leaf");
}

/*************/
TEST_CASE("Testing the Leaf's callbacks")
{
    Tree::Root maple;
    maple.createLeafAt("/a_leaf");
    CHECK(maple.hasLeafAt("/a_leaf"));

    Value extValue{""};
    auto callbackID = maple.addCallbackToLeafAt("/a_leaf", [&extValue](Value value, chrono::system_clock::time_point timestamp) { extValue = value; });
    CHECK(callbackID > 0);
    maple.setValueForLeafAt("/a_leaf", "Ceci n'est pas un test");

    CHECK(extValue == Value("Ceci n'est pas un test"));
    CHECK(maple.removeCallbackFromLeafAt("/a_leaf", callbackID));

    maple.setValueForLeafAt("/a_leaf", "Ceci non plus");
    CHECK(extValue == Value("Ceci n'est pas un test"));
}

/*************/
TEST_CASE("Testing propagation through a main tree")
{
    Tree::Root main, maple, oak;
    maple.createLeafAt("/some_leaf");
    maple.createBranchAt("/a_branch");
    auto seeds = maple.getSeedList();
    main.addSeedsToQueue(seeds);
    main.processQueue(true);
    seeds = main.getSeedList();
    oak.addSeedsToQueue(seeds);
    oak.processQueue();

    CHECK(main == maple);
    CHECK(main == oak);
}

/*************/
TEST_CASE("Testing RootHandle")
{
    Tree::Root main;
    main.createLeafAt("/first_leaf");
    {
        auto handle = main.getHandle();
        handle->createBranchAt("/first_branch");
        CHECK(handle->hasLeafAt("/first_leaf"));
    }
    CHECK(main.hasBranchAt("/first_branch"));
}
