#include "./core/tree/tree_branch.h"

#include "./utils/log.h"

#include "./core/tree/tree_leaf.h"

namespace Splash
{

namespace Tree
{

/*************/
Branch::Branch(const std::string& name, Branch* parent)
    : _name(name)
    , _parentBranch(parent)
{
}

/*************/
bool Branch::operator==(const Branch& rhs) const
{
    if (_branches.size() != rhs._branches.size())
        return false;

    if (_leaves.size() != rhs._leaves.size())
        return false;

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
bool Branch::addBranch(std::unique_ptr<Branch>&& branch)
{
    if (!branch)
        return false;

    auto branchName = branch->getName();
    if (_branches.find(branchName) != _branches.end())
        return false;

    branch->setParent(this);
    _branches.emplace(branchName, std::move(branch));

    for (const auto& id : _callbackTargetIds[Task::AddBranch])
        _callbacks[id](*this, branchName);

    return true;
}

/*************/
bool Branch::addLeaf(std::unique_ptr<Leaf>&& leaf)
{
    if (!leaf)
        return false;

    auto leafName = leaf->getName();
    if (_leaves.find(leafName) != _leaves.end())
        return false;

    leaf->setParent(this);
    _leaves.emplace(leafName, std::move(leaf));

    for (const auto& id : _callbackTargetIds[Task::AddLeaf])
        _callbacks[id](*this, leafName);

    return true;
}

/*************/
Branch::UpdateCallbackID Branch::addCallback(Task target, const UpdateCallback& callback)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    auto id = ++_currentCallbackID;
    _callbackTargetIds[target].emplace(id);
    _callbacks[id] = callback;
    return id;
}

/*************/
bool Branch::removeCallback(int id)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    auto callbackIt = _callbacks.find(id);
    if (callbackIt == _callbacks.end())
        return false;
    _callbacks.erase(callbackIt);
    for (auto& ids : _callbackTargetIds)
        ids.second.erase(id);
    return true;
}

/*************/
std::unique_ptr<Branch> Branch::cutBranch(const std::string& branchName)
{
    auto branchIt = _branches.find(branchName);
    if (branchIt == _branches.end())
        return {nullptr};

    std::unique_ptr<Branch> branch{nullptr};
    swap(branchIt->second, branch);
    _branches.erase(branchIt);
    branch->setParent(nullptr);
    return branch;
}

/*************/
std::unique_ptr<Leaf> Branch::cutLeaf(const std::string& leafName)
{
    auto leafIt = _leaves.find(leafName);
    if (leafIt == _leaves.end())
        return {nullptr};

    std::unique_ptr<Leaf> leaf{nullptr};
    swap(leafIt->second, leaf);
    _leaves.erase(leafIt);
    leaf->setParent(nullptr);
    return leaf;
}

/*************/
Branch* Branch::getBranch(const std::string& path)
{
    auto branchIt = _branches.find(path);
    if (branchIt == _branches.end())
        return nullptr;

    return branchIt->second.get();
}

/*************/
std::list<std::string> Branch::getBranchList() const
{
    std::list<std::string> branchList;
    for (const auto& branch : _branches)
        branchList.push_back(branch.first);
    return branchList;
}

/*************/
Leaf* Branch::getLeaf(const std::string& path)
{
    auto leafIt = _leaves.find(path);
    if (leafIt == _leaves.end())
        return nullptr;

    return leafIt->second.get();
}

/*************/
std::list<std::string> Branch::getLeafList() const
{
    std::list<std::string> leafList;
    for (const auto& leaf : _leaves)
        leafList.push_back(leaf.first);
    return leafList;
}

/*************/
std::string Branch::getPath() const
{
    if (_parentBranch)
        return _parentBranch->getPath() + _name + "/";
    else
        return "/";
}

/*************/
std::string Branch::print(int indent) const
{
    std::string description;
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
bool Branch::removeBranch(const std::string& name)
{
    if (!hasBranch(name))
        return false;

    for (const auto& id : _callbackTargetIds[Task::RemoveBranch])
        _callbacks[id](*this, name);

    _branches[name]->setParent(nullptr);
    _branches.erase(name);

    return true;
}

/*************/
bool Branch::removeLeaf(const std::string& name)
{
    if (!hasLeaf(name))
        return false;

    for (const auto& id : _callbackTargetIds[Task::RemoveLeaf])
        _callbacks[id](*this, name);

    _leaves[name]->setParent(nullptr);
    _leaves.erase(name);
    return true;
}

/*************/
bool Branch::renameBranch(const std::string& name, const std::string& newName)
{
    if (_branches.find(name) == _branches.end())
        return false;

    if (_branches.find(newName) != _branches.end())
        return false;

    auto branchIt = _branches.find(name);
    branchIt->second->setName(newName);
    _branches.emplace(newName, std::move(branchIt->second));
    _branches.erase(name);
    return true;
}

/*************/
bool Branch::renameLeaf(const std::string& name, const std::string& newName)
{
    if (_leaves.find(newName) != _leaves.end())
        return false;

    auto leafIt = _leaves.find(name);
    leafIt->second->setName(newName);
    _leaves.emplace(newName, std::move(leafIt->second));
    _leaves.erase(leafIt);
    return true;
}

} // namespace Tree

} // namespace Splash
