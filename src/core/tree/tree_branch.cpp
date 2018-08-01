#include "./core/tree/tree_branch.h"

#include "./utils/log.h"

#include "./core/tree/tree_leaf.h"

using namespace std;

namespace Splash
{

namespace Tree
{

/*************/
Branch::Branch(const string& name, Branch* parent)
    : _name(name)
    , _parentBranch(parent)
{
}

/*************/
bool Branch::operator==(const Branch& rhs) const
{
    for (const auto& it : _branches)
    {
        auto rhsIt = rhs._branches.find(it.first);
        if (rhsIt == rhs._branches.end())
            return false;
        if (*it.second != *rhsIt->second)
            return false;
    }

    for (const auto& it : _leaves)
    {
        auto rhsIt = rhs._leaves.find(it.first);
        if (rhsIt == rhs._leaves.end())
            return false;
        if (*it.second != *rhsIt->second)
            return false;
    }

    return true;
}

/*************/
bool Branch::addBranch(unique_ptr<Branch>&& branch)
{
    if (!branch)
        return false;

    if (_branches.find(branch->getName()) != _branches.end())
        return false;

    branch->setParent(this);
    _branches.emplace(make_pair(branch->getName(), move(branch)));
    return true;
}

/*************/
bool Branch::addLeaf(unique_ptr<Leaf>&& leaf)
{
    if (!leaf)
        return false;

    if (_leaves.find(leaf->getName()) != _leaves.end())
        return false;

    leaf->setParent(this);
    _leaves.emplace(make_pair(leaf->getName(), move(leaf)));
    return true;
}

/*************/
Branch* Branch::getBranch(const string& path)
{
    auto branchIt = _branches.find(path);
    if (branchIt == _branches.end())
        return {nullptr};

    return branchIt->second.get();
}

/*************/
list<string> Branch::getBranchList() const
{
    list<string> branchList;
    for (const auto& branch : _branches)
        branchList.push_back(branch.first);
    return branchList;
}

/*************/
Leaf* Branch::getLeaf(const string& path)
{
    auto leafIt = _leaves.find(path);
    if (leafIt == _leaves.end())
        return {nullptr};

    return leafIt->second.get();
}

/*************/
list<string> Branch::getLeafList() const
{
    list<string> leafList;
    for (const auto& leaf : _leaves)
        leafList.push_back(leaf.first);
    return leafList;
}

/*************/
list<string> Branch::getLeafNames() const
{
    list<string> leafNames{};
    for (const auto& leaf : _leaves)
        leafNames.push_back(leaf.second->getName());
    return leafNames;
}

/*************/
string Branch::getPath() const
{
    string path{};
    if (_parentBranch)
        path = _parentBranch->getPath() + _name + "/";
    else
        path = "/";

    return path;
}

/*************/
string Branch::print(int indent) const
{
    string description;
    for (int i = 0; i < indent / 2; ++i)
        description += "| ";
    description += "|-- " + _name + "\n";

    for (const auto& branch : _branches)
        description += branch.second->print(indent + 2);

    for (const auto& leaf : _leaves)
        description += leaf.second->print(indent + 2);

    return description;
}

/*************/
bool Branch::removeBranch(string name)
{
    if (!hasBranch(name))
        return false;

    _branches[name]->setParent(nullptr);
    _branches.erase(name);
    return true;
}

/*************/
bool Branch::removeLeaf(string name)
{
    if (!hasLeaf(name))
        return false;

    _leaves[name]->setParent(nullptr);
    _leaves.erase(name);
    return true;
}

} // namespace Tree

} // namespace Splash
