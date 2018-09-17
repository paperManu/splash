#include "./core/tree/tree_root.h"

#include <stdexcept>

#include "./utils/log.h"

using namespace std;

namespace Splash
{

namespace Tree
{

/*************/
RootHandle::RootHandle(Root* root)
    : _root(root)
    , _rootLock(root->_treeMutex)
{
}

/*************/
Root::Root()
    : _rootBranch(new Branch(""))
{
}

/*************/
bool Root::operator==(const Root& rhs) const
{
    return (*_rootBranch == *rhs._rootBranch);
}

/*************/
bool Root::addBranchAt(const string& path, unique_ptr<Branch>&& branch, bool silent)
{
    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(processPath(path));
    if (!holdingBranch)
        return false;

    auto branchPath = holdingBranch->getPath() + branch->getName();
    if (!holdingBranch->addBranch(move(branch)))
        return false;

    if (!silent)
    {
        auto seeds = generateSeedsForBranch(getBranchAt(branchPath));
        lock_guard<recursive_mutex> lock(_updatesMutex);
        _updates.merge(seeds, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    }

    return true;
}

/*************/
bool Root::addLeafAt(const string& path, unique_ptr<Leaf>&& leaf, bool silent)
{
    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(processPath(path));
    if (!holdingBranch)
        return false;

    auto leafPath = holdingBranch->getPath() + leaf->getName();
    if (!holdingBranch->addLeaf(move(leaf)))
        return false;

    if (!silent)
    {
        auto seeds = generateSeedsForLeaf(getLeafAt(leafPath));
        lock_guard<recursive_mutex> lock(_updatesMutex);
        _updates.merge(seeds, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    }

    return true;
}

/*************/
Branch::UpdateCallbackID Root::addCallbackToBranchAt(const string& path, Branch::Task target, const Branch::UpdateCallback& cb, bool pending)
{
    auto branch = getBranchAt(path);
    if (!branch && !pending)
        return 0;

    if (branch)
        return branch->addCallback(target, cb);

    _branchCallbacksToRegister.push_back(make_tuple(path, target, cb));
    return -1;
}

/*************/
Leaf::UpdateCallbackID Root::addCallbackToLeafAt(const string& path, const Leaf::UpdateCallback& cb, bool pending)
{
    auto leaf = getLeafAt(path);
    if (!leaf && !pending)
        return 0;

    if (leaf)
        return leaf->addCallback(cb);

    _leafCallbacksToRegister.push_back(make_pair(path, cb));
    return -1;
}

/*************/
bool Root::removeCallbackFromBranchAt(const string& path, int id)
{
    auto branch = getBranchAt(path);
    if (!branch)
        return false;

    return branch->removeCallback(id);
}

/*************/
bool Root::removeCallbackFromLeafAt(const string& path, int id)
{
    auto leaf = getLeafAt(path);
    if (!leaf)
        return false;

    return leaf->removeCallback(id);
}

/*************/
void Root::addSeedToQueue(Tree::Task taskType, Values args, chrono::system_clock::time_point timestamp)
{
    auto seed = make_tuple(taskType, args, timestamp, UUID(false));
    lock_guard<mutex> lock(_taskMutex);
    _seedQueue.push_back(seed);
}

/*************/
void Root::addSeedsToQueue(list<Seed> seeds)
{
    lock_guard<mutex> lock(_taskMutex);
    _seedQueue.merge(seeds, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
}

/*************/
void Root::clearSeedList()
{
    lock_guard<recursive_mutex> lock(_updatesMutex);
    _updates.clear();
}

/*************/
void Root::cutdown()
{
    _rootBranch = make_unique<Tree::Branch>("");
    _seedQueue.clear();
    _updates.clear();
    _branchCallbacksToRegister.clear();
    _leafCallbacksToRegister.clear();
}

/*************/
bool Root::createBranchAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto newBranchName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
        string parentPath;
        for (const auto& part : parts)
            parentPath += "/" + part;

        if (!createBranchAt(parentPath, silent))
            return false;
        assert(getBranchAt(parts));
        holdingBranch = getBranchAt(parts);
    }

    auto newBranch = make_unique<Branch>(newBranchName);
    if (!holdingBranch->addBranch(move(newBranch)))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " already has a branch named " << newBranchName << Log::endl;
#endif
        return false;
    }

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::AddBranch, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
bool Root::createLeafAt(const std::string& path, Values value, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto newLeafName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
        string parentPath;
        for (const auto& part : parts)
            parentPath += "/" + part;

        if (!createBranchAt(parentPath, silent))
            return false;
        assert(getBranchAt(parts));
        holdingBranch = getBranchAt(parts);
    }

    auto newLeaf = make_unique<Leaf>(newLeafName, value);
    if (!holdingBranch->addLeaf(move(newLeaf)))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " already has a leaf named " << newLeafName << Log::endl;
#endif
        return false;
    }

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::AddLeaf, Values({path, value}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
unique_ptr<Branch> Root::cutBranchAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return nullptr;
    }

    auto branchName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return nullptr;

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        _updates.emplace_back(make_tuple(Task::RemoveBranch, Values({path}), chrono::system_clock::now(), _uuid));
    }

    return holdingBranch->cutBranch(branchName);
}

/*************/
unique_ptr<Leaf> Root::cutLeafAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return nullptr;
    }

    auto leafName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return nullptr;

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        _updates.emplace_back(make_tuple(Task::RemoveLeaf, Values({path}), chrono::system_clock::now(), _uuid));
    }

    return holdingBranch->cutLeaf(leafName);
}

/*************/
bool Root::getError(string& error)
{
    if (!_hasError)
    {
        error = "";
        return false;
    }

    _hasError = false;
    error = _errorMsg;

    _errorMsg = "";
    return true;
}

/*************/
list<string> Root::getBranchListAt(const string& path) const
{
    auto parts = processPath(path);
    if (path != "/" && parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return {};
    }

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return {};

    return holdingBranch->getBranchList();
}

/*************/
list<string> Root::getLeafListAt(const string& path) const
{
    auto parts = processPath(path);
    if (path != "/" && parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return {};
    }

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return {};

    return holdingBranch->getLeafList();
}

/*************/
bool Root::getValueForLeafAt(const string& path, Value& value) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto leafName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    auto leaf = holdingBranch->getLeaf(leafName);
    if (!leaf)
        return false;

    value = leaf->get();
    return true;
}

/*************/
bool Root::hasBranchAt(const string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto branchName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (holdingBranch->hasBranch(branchName))
        return true;
    else
        return false;
}

/*************/
bool Root::hasLeafAt(const string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto leafName = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (holdingBranch->hasLeaf(leafName))
        return true;
    else
        return false;
}

/*************/
bool Root::setValueForLeafAt(const string& path, const Value& value, int64_t timestamp, bool silent)
{
    chrono::system_clock::time_point timePoint;
    if (timestamp)
        timePoint = chrono::system_clock::time_point(chrono::milliseconds(timestamp));
    else
        timePoint = chrono::system_clock::now();

    return setValueForLeafAt(path, value, timePoint, silent);
}

/*************/
bool Root::setValueForLeafAt(const string& path, const Value& value, chrono::system_clock::time_point timestamp, bool silent)
{
    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto leaf = getLeafAt(processPath(path));
    if (!leaf)
        return false;

    if (value == leaf->get())
        return true;

    if (!leaf->set(value, timestamp))
        return false;

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::SetLeaf, Values({path, value}), timestamp, _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
list<Seed> Root::getSeedList()
{
    list<Seed> updates;
    lock_guard<recursive_mutex> lock(_updatesMutex);
    swap(updates, _updates);
    return updates;
}

/*************/
string Root::print() const
{
    if (!_rootBranch)
        return {};

    return _rootBranch->print(0);
}

/*************/
bool Root::processQueue(bool propagate)
{
    _taskMutex.lock();
    list<Seed> tasks;
    swap(tasks, _seedQueue);
    _taskMutex.unlock();

    tasks.sort([](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    for (const auto& seed : tasks)
    {
        auto& task = std::get<0>(seed);
        auto args = std::get<1>(seed).as<Values>();
        auto& timestamp = std::get<2>(seed);
        auto& sourceTreeUUID = std::get<3>(seed);

        if (sourceTreeUUID == _uuid)
            continue;

        switch (task)
        {
        default:
        {
            if (!_hasError)
            {
                _hasError = true;
                _errorMsg = "Task type not recognized";
            }
            throw runtime_error(_errorMsg);
        }
        case Task::AddBranch:
        {
            if (args.size() != 1)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task AddBranch";
                throw runtime_error(errorStr);
            }

            auto branchPath = args[0].as<string>();
            if (!createBranchAt(branchPath, true))
            {
                _hasError = true;
                _errorMsg = "Could not add branch " + branchPath;
                break;
            }
            break;
        }
        case Task::AddLeaf:
        {
            if (args.size() < 1)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task AddLeaf";
                throw runtime_error(errorStr);
            }

            auto leafPath = args[0].as<string>();
            auto values = args.size() == 2 ? args[1].as<Values>() : Values();
            if (createLeafAt(leafPath, values, true))
            {
                _hasError = true;
                _errorMsg = "Could not add leaf " + leafPath;
                break;
            }
            break;
        }
        case Task::RemoveBranch:
        {
            if (args.size() != 1)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task RemoveBranch";
                throw runtime_error(errorStr);
            }

            auto branchPath = args[0].as<string>();
            if (!removeBranchAt(branchPath, true))
            {
                _hasError = true;
                _errorMsg = "Could not remove branch " + branchPath;
                break;
            }
            break;
        }
        case Task::RemoveLeaf:
        {
            if (args.size() != 1)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task RemoveLeaf";
                throw runtime_error(errorStr);
            }

            auto leafPath = args[0].as<string>();
            if (!removeLeafAt(leafPath, true))
            {
                _hasError = true;
                _errorMsg = "Could not remove leaf " + leafPath;
                break;
            }
            break;
        }
        case Task::RenameBranch:
        {
            if (args.size() != 2)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task RenamedBranch";
                throw runtime_error(errorStr);
            }

            auto branchPath = args[0].as<string>();
            auto branchName = args[1].as<string>();
            if (!renameBranchAt(branchPath, branchName, true))
            {
                _hasError = true;
                _errorMsg = "Could not rename branch " + branchPath;
                break;
            }
            break;
        }
        case Task::RenameLeaf:
        {
            if (args.size() != 2)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task RenamedLeaf";
                throw runtime_error(errorStr);
            }

            auto leafPath = args[0].as<string>();
            auto leafName = args[1].as<string>();
            if (!renameLeafAt(leafPath, leafName, true))
            {
                _hasError = true;
                _errorMsg = "Could not rename leaf " + leafPath;
                break;
            }
            break;
        }
        case Task::SetLeaf:
        {
            if (args.size() < 1)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task SetLeaf";
                throw runtime_error(errorStr);
            }
            auto leafPath = args[0].as<string>();
            auto& value = args[1];
            if (!setValueForLeafAt(leafPath, value, timestamp, true))
            {
                _hasError = true;
                _errorMsg = "Error while setting leaf at path " + leafPath;
                break;
            }

            break;
        }
        }
    }

    if (propagate)
        _updates.merge(tasks, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

    registerPendingCallbacks();
    return true;
}

/*************/
bool Root::removeBranchAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto branchToRemove = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Could not find branch holding path " << path << Log::endl;
#endif
        return false;
    }

    if (!holdingBranch->removeBranch(branchToRemove))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " does not have a branch named " << branchToRemove
                   << Log::endl;
#endif
        return false;
    }

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::RemoveBranch, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
bool Root::removeLeafAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto leafToRemove = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Could not find branch holding leaf " << path << Log::endl;
#endif
        return false;
    }

    if (!holdingBranch->removeLeaf(leafToRemove))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " does not have a leaf named " << leafToRemove << Log::endl;
#endif
        return false;
    }

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::RemoveLeaf, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
bool Root::renameBranchAt(const string& path, const string& name, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto branchToRename = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Could not find branch holding path " << path << Log::endl;
#endif
        return false;
    }

    if (!holdingBranch->renameBranch(branchToRename, name))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Could not rename branch " << path << " to " << name << Log::endl;
#endif
        return false;
    }

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::RenameBranch, Values({path, name}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
bool Root::renameLeafAt(const string& path, const string& name, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return false;
    }

    auto leafToRename = parts.back();
    parts.pop_back();

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Could not find branch holding path " << path << Log::endl;
#endif
        return false;
    }

    if (!holdingBranch->renameLeaf(leafToRename, name))
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Could not rename leaf " << path << " to " << name << Log::endl;
#endif
        return false;
    }

    if (!silent)
    {
        lock_guard<recursive_mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::RenameLeaf, Values({path, name}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
list<Seed> Root::generateSeedsForBranch(Branch* branch)
{
    list<Seed> seeds;
    if (!branch)
        return seeds;

    lock_guard<recursive_mutex> lockUpdates(_updatesMutex);
    seeds.emplace_back(make_tuple(Task::AddBranch, Values({branch->getPath()}), chrono::system_clock::now(), _uuid));
    for (const auto& branchName : branch->getBranchList())
    {
        auto childBranch = getBranchAt(branch->getPath() + branchName);
        seeds.merge(generateSeedsForBranch(childBranch), [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    }

    for (const auto& leafName : branch->getLeafList())
    {
        auto leaf = getLeafAt(branch->getPath() + leafName);
        seeds.merge(generateSeedsForLeaf(leaf), [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    }

    return seeds;
}

/*************/
list<Seed> Root::generateSeedsForLeaf(Leaf* leaf)
{
    list<Seed> seeds;
    if (!leaf)
        return seeds;

    lock_guard<recursive_mutex> lockUpdates(_updatesMutex);
    seeds.emplace_back(make_tuple(Task::AddLeaf, Values({leaf->getPath()}), chrono::system_clock::now(), _uuid));
    seeds.emplace_back(make_tuple(Task::SetLeaf, Values({leaf->getPath(), leaf->get()}), chrono::system_clock::now(), _uuid));
    return seeds;
}

/*************/
list<Seed> Root::getSeedsForPath(const string& path)
{
    lock_guard<recursive_mutex> lockTree(_treeMutex);

    if (hasLeafAt(path))
        return generateSeedsForLeaf(getLeafAt(path));
    else if (hasBranchAt(path))
        return generateSeedsForBranch(getBranchAt(path));

    return {};
}

/*************/
Branch* Root::getBranchAt(const list<string>& path) const
{
    Branch* branch = _rootBranch.get();
    for (const auto& part : path)
    {
        auto childBranch = branch->getBranch(part);
        if (!childBranch)
        {
#ifdef DEBUG
            Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << branch->getPath() << " does not have branch " << part << Log::endl;
#endif
            return nullptr;
        }
        branch = childBranch;
    }

    return branch;
}

/*************/
Branch* Root::getBranchAt(const string& path) const
{
    auto parts = processPath(path);
    lock_guard<recursive_mutex> lockTree(_treeMutex);
    return getBranchAt(parts);
}

/*************/
Leaf* Root::getLeafAt(const string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return nullptr;
    }

    lock_guard<recursive_mutex> lockTree(_treeMutex);
    return getLeafAt(parts);
}

/*************/
Leaf* Root::getLeafAt(const list<string>& path) const
{
    Branch* branch = _rootBranch.get();
    auto leafName = path.back();

    float index = 0;
    for (const auto& part : path)
    {
        if (index++ == path.size() - 1)
            break;

        auto childBranch = branch->getBranch(part);
        if (!childBranch)
        {
#ifdef DEBUG
            Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << branch->getPath() << " does not have branch " << part << Log::endl;
#endif
            return nullptr;
        }
        branch = childBranch;
    }

    auto leaf = branch->getLeaf(leafName);
    if (!leaf)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Branch " << branch->getPath() << " does not have a leaf name " << leafName << Log::endl;
#endif
        return nullptr;
    }

    return leaf;
}

/*************/
list<string> Root::processPath(const string& path)
{
    if (path[0] != '/')
        throw invalid_argument("Tree path should start with a '/'");

    auto subpath = path;
    if (subpath[subpath.size() - 1] == '/')
        subpath = subpath.substr(1, subpath.size() - 1);
    else
        subpath = path.substr(1);

    list<string> parts;
    while (true)
    {
        auto pos = subpath.find("/");
        if (pos != string::npos)
        {
            parts.push_back(subpath.substr(0, pos));
            subpath = subpath.substr(pos + 1);
        }
        else
        {
            subpath = subpath.substr(0, pos);
            if (!subpath.empty())
                parts.push_back(subpath);
            break;
        }
    }

    return parts;
}

/*************/
void Root::registerPendingCallbacks()
{
    _branchCallbacksToRegister.remove_if([this](const tuple<string, Branch::Task, Branch::UpdateCallback>& callback) {
        return addCallbackToBranchAt(std::get<0>(callback), std::get<1>(callback), std::get<2>(callback));
    });
    _leafCallbacksToRegister.remove_if([this](const pair<string, Leaf::UpdateCallback>& callback) { return addCallbackToLeafAt(callback.first, callback.second); });
}

} // namespace Tree

} // namespace Splash
