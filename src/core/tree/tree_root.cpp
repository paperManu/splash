#include "./core/tree/tree_root.h"

#include <stdexcept>

#include "./utils/log.h"

namespace chrono = std::chrono;

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
bool Root::addBranchAt(const std::string& path, std::unique_ptr<Branch>&& branch, bool silent)
{
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(processPath(path));
    if (!holdingBranch)
        return false;

    auto branchPath = holdingBranch->getPath() + branch->getName();
    if (!holdingBranch->addBranch(std::move(branch)))
        return false;

    if (!silent)
    {
        auto seeds = generateSeedsForBranch(getBranchAt(branchPath));
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        _updates.merge(seeds, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    }

    return true;
}

/*************/
bool Root::addLeafAt(const std::string& path, std::unique_ptr<Leaf>&& leaf, bool silent)
{
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(processPath(path));
    if (!holdingBranch)
        return false;

    auto leafPath = holdingBranch->getPath() + leaf->getName();
    if (!holdingBranch->addLeaf(std::move(leaf)))
        return false;

    if (!silent)
    {
        auto seeds = generateSeedsForLeaf(getLeafAt(leafPath));
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        _updates.merge(seeds, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    }

    return true;
}

/*************/
Branch::UpdateCallbackID Root::addCallbackToBranchAt(const std::string& path, Branch::Task target, const Branch::UpdateCallback& cb, bool pending)
{
    auto branch = getBranchAt(path);
    if (!branch && !pending)
        return 0;

    if (branch)
        return branch->addCallback(target, cb);

    _branchCallbacksToRegister.push_back(std::make_tuple(path, target, cb));
    return -1;
}

/*************/
Leaf::UpdateCallbackID Root::addCallbackToLeafAt(const std::string& path, const Leaf::UpdateCallback& cb, bool pending)
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
bool Root::removeCallbackFromBranchAt(const std::string& path, int id)
{
    auto branch = getBranchAt(path);
    if (!branch)
        return false;

    return branch->removeCallback(id);
}

/*************/
bool Root::removeCallbackFromLeafAt(const std::string& path, int id)
{
    auto leaf = getLeafAt(path);
    if (!leaf)
        return false;

    return leaf->removeCallback(id);
}

/*************/
void Root::addSeedToQueue(Tree::Task taskType, Values args, chrono::system_clock::time_point timestamp)
{
    auto seed = std::make_tuple(taskType, args, timestamp, UUID(false));
    std::lock_guard<std::mutex> lock(_taskMutex);
    _seedQueue.push_back(seed);
}

/*************/
void Root::addSeedsToQueue(std::list<Seed> seeds)
{
    std::lock_guard<std::mutex> lock(_taskMutex);
    _seedQueue.merge(seeds, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
}

/*************/
void Root::clearSeedList()
{
    std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
    _updates.clear();
}

/*************/
void Root::cutdown()
{
    _rootBranch = std::make_unique<Tree::Branch>("");
    _seedQueue.clear();
    _updates.clear();
    _branchCallbacksToRegister.clear();
    _leafCallbacksToRegister.clear();
}

/*************/
bool Root::createBranchAt(const std::string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        return false;
    }

    auto newBranchName = parts.back();
    parts.pop_back();

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
        std::string parentPath;
        for (const auto& part : parts)
            parentPath += "/" + part;

        if (!createBranchAt(parentPath, silent))
            return false;
        assert(getBranchAt(parts));
        holdingBranch = getBranchAt(parts);
        assert(holdingBranch);
    }

    auto newBranch = std::make_unique<Branch>(newBranchName);
    if (!holdingBranch->addBranch(std::move(newBranch)))
    {
        return false;
    }

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        auto seed = std::make_tuple(Task::AddBranch, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(std::move(seed));
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
        std::string parentPath;
        for (const auto& part : parts)
            parentPath += "/" + part;

        if (!createBranchAt(parentPath, silent))
            return false;
        assert(getBranchAt(parts));
        holdingBranch = getBranchAt(parts);
        assert(holdingBranch);
    }

    auto newLeaf = std::make_unique<Leaf>(newLeafName, value);
    if (!holdingBranch->addLeaf(std::move(newLeaf)))
        return false;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        auto seed = std::make_tuple(Task::AddLeaf, Values({path, value}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(std::move(seed));
    }

    return true;
}

/*************/
std::unique_ptr<Branch> Root::cutBranchAt(const std::string& path, bool silent)
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return nullptr;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        _updates.emplace_back(std::make_tuple(Task::RemoveBranch, Values({path}), chrono::system_clock::now(), _uuid));
    }

    return holdingBranch->cutBranch(branchName);
}

/*************/
std::unique_ptr<Leaf> Root::cutLeafAt(const std::string& path, bool silent)
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return nullptr;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        _updates.emplace_back(std::make_tuple(Task::RemoveLeaf, Values({path}), chrono::system_clock::now(), _uuid));
    }

    return holdingBranch->cutLeaf(leafName);
}

/*************/
bool Root::getError(std::string& error)
{
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);

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
std::list<std::string> Root::getBranchListAt(const std::string& path) const
{
    auto parts = processPath(path);
    if (path != "/" && parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return {};
    }

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return {};

    return holdingBranch->getBranchList();
}

/*************/
std::list<std::string> Root::getLeafListAt(const std::string& path) const
{
    auto parts = processPath(path);
    if (path != "/" && parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return {};
    }

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return {};

    return holdingBranch->getLeafList();
}

/*************/
bool Root::getValueForLeafAt(const std::string& path, Value& value) const
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
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
bool Root::hasBranchAt(const std::string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
        return true; // The root is always there!

    auto branchName = parts.back();
    parts.pop_back();

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (holdingBranch->hasBranch(branchName))
        return true;
    else
        return false;
}

/*************/
bool Root::hasLeafAt(const std::string& path) const
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (holdingBranch->hasLeaf(leafName))
        return true;
    else
        return false;
}

/*************/
bool Root::setValueForLeafAt(const std::string& path, const Value& value, int64_t timestamp, bool force)
{
    chrono::system_clock::time_point timePoint;
    if (timestamp)
        timePoint = chrono::system_clock::time_point(chrono::milliseconds(timestamp));
    else
        timePoint = chrono::system_clock::now();

    return setValueForLeafAt(path, value, timePoint, force);
}

/*************/
bool Root::setValueForLeafAt(const std::string& path, const Value& value, chrono::system_clock::time_point timestamp, bool force)
{
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto leaf = getLeafAt(processPath(path));
    if (!leaf)
        return false;

    if (!force && value == leaf->get())
        return true;

    if (!writeValueToLeafAt(path, value, timestamp))
        return false;

    std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
    auto seed = std::make_tuple(Task::SetLeaf, Values({path, value}), timestamp, _uuid);
    _updates.emplace_back(std::move(seed));

    return true;
}

/*************/
bool Root::writeValueToLeafAt(const std::string& path, const Value& value, chrono::system_clock::time_point timestamp)
{
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto leaf = getLeafAt(processPath(path));
    if (!leaf)
        return false;

    if (!leaf->set(value, timestamp))
        return false;

    return true;
}

/*************/
std::list<Seed> Root::getUpdateSeedList()
{
    std::list<Seed> updates;
    std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
    swap(updates, _updates);
    return updates;
}

/*************/
std::string Root::print() const
{
    if (!_rootBranch)
        return {};

    return _rootBranch->print(0);
}

/*************/
bool Root::processQueue(bool propagate)
{
    _taskMutex.lock();
    std::list<Seed> tasks;
    swap(tasks, _seedQueue);
    _taskMutex.unlock();

    tasks.sort([](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
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
            throw std::runtime_error(_errorMsg);
        }
        case Task::AddBranch:
        {
            if (args.size() != 1)
            {
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task AddBranch";
                throw std::runtime_error(errorStr);
            }

            auto branchPath = args[0].as<std::string>();
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
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task AddLeaf";
                throw std::runtime_error(errorStr);
            }

            auto leafPath = args[0].as<std::string>();
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
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task RemoveBranch";
                throw std::runtime_error(errorStr);
            }

            auto branchPath = args[0].as<std::string>();
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
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task RemoveLeaf";
                throw std::runtime_error(errorStr);
            }

            auto leafPath = args[0].as<std::string>();
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
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task RenamedBranch";
                throw std::runtime_error(errorStr);
            }

            auto branchPath = args[0].as<std::string>();
            auto branchName = args[1].as<std::string>();
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
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task RenamedLeaf";
                throw std::runtime_error(errorStr);
            }

            auto leafPath = args[0].as<std::string>();
            auto leafName = args[1].as<std::string>();
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
                auto errorStr = "Root::" + std::string(__FUNCTION__) + " - Wrong number of arguments for task SetLeaf";
                throw std::runtime_error(errorStr);
            }
            auto leafPath = args[0].as<std::string>();
            auto& value = args[1];
            if (!writeValueToLeafAt(leafPath, value, timestamp))
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
bool Root::removeBranchAt(const std::string& path, bool silent)
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (!holdingBranch->removeBranch(branchToRemove))
        return false;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        auto seed = std::make_tuple(Task::RemoveBranch, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(std::move(seed));
    }

    return true;
}

/*************/
bool Root::removeLeafAt(const std::string& path, bool silent)
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (!holdingBranch->removeLeaf(leafToRemove))
        return false;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        auto seed = std::make_tuple(Task::RemoveLeaf, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(std::move(seed));
    }

    return true;
}

/*************/
bool Root::renameBranchAt(const std::string& path, const std::string& name, bool silent)
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (!holdingBranch->renameBranch(branchToRename, name))
        return false;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        auto seed = std::make_tuple(Task::RenameBranch, Values({path, name}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(std::move(seed));
    }

    return true;
}

/*************/
bool Root::renameLeafAt(const std::string& path, const std::string& name, bool silent)
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

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (!holdingBranch->renameLeaf(leafToRename, name))
        return false;

    if (!silent)
    {
        std::lock_guard<std::recursive_mutex> lock(_updatesMutex);
        auto seed = std::make_tuple(Task::RenameLeaf, Values({path, name}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(std::move(seed));
    }

    return true;
}

/*************/
std::list<Seed> Root::generateSeedsForBranch(Branch* branch)
{
    std::list<Seed> seeds;
    if (!branch)
        return seeds;

    std::lock_guard<std::recursive_mutex> lockUpdates(_updatesMutex);
    seeds.emplace_back(std::make_tuple(Task::AddBranch, Values({branch->getPath()}), chrono::system_clock::now(), _uuid));
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
std::list<Seed> Root::generateSeedsForLeaf(Leaf* leaf)
{
    std::list<Seed> seeds;
    if (!leaf)
        return seeds;

    std::lock_guard<std::recursive_mutex> lockUpdates(_updatesMutex);
    seeds.emplace_back(std::make_tuple(Task::AddLeaf, Values({leaf->getPath()}), chrono::system_clock::now(), _uuid));
    seeds.emplace_back(std::make_tuple(Task::SetLeaf, Values({leaf->getPath(), leaf->get()}), chrono::system_clock::now(), _uuid));
    return seeds;
}

/*************/
std::list<Seed> Root::getSeedsForPath(const std::string& path)
{
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);

    if (hasLeafAt(path))
        return generateSeedsForLeaf(getLeafAt(path));
    else if (hasBranchAt(path))
        return generateSeedsForBranch(getBranchAt(path));

    return {};
}

/*************/
Branch* Root::getBranchAt(const std::vector<std::string>& path) const
{
    Branch* branch = _rootBranch.get();
    for (const auto& part : path)
    {
        auto childBranch = branch->getBranch(part);
        if (!childBranch)
            return nullptr;
        branch = childBranch;
    }

    return branch;
}

/*************/
Branch* Root::getBranchAt(const std::string& path) const
{
    auto parts = processPath(path);
    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    return getBranchAt(parts);
}

/*************/
Leaf* Root::getLeafAt(const std::string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
#endif
        return nullptr;
    }

    std::lock_guard<std::recursive_mutex> lockTree(_treeMutex);
    return getLeafAt(parts);
}

/*************/
Leaf* Root::getLeafAt(const std::vector<std::string>& path) const
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
            return nullptr;
        branch = childBranch;
    }

    auto leaf = branch->getLeaf(leafName);
    if (!leaf)
        return nullptr;

    return leaf;
}

/*************/
std::vector<std::string> Root::processPath(const std::string& path)
{
    if (path[0] != '/')
        throw std::invalid_argument("Tree path should start with a '/'");

    size_t start = 1;
    std::vector<std::string> parts;
    parts.reserve(8);
    while (true)
    {
        size_t end = path.find('/', start);
        if (end - start == 0)
        {
            start++;
        }
        else if (end != std::string::npos)
        {
            parts.push_back(path.substr(start, end - start));
            start = end + 1;
        }
        else
        {
            if (start != path.size())
                parts.push_back(path.substr(start, path.size()));
            break;
        }
    }

    return parts;
}

/*************/
void Root::registerPendingCallbacks()
{
    _branchCallbacksToRegister.remove_if([this](const std::tuple<std::string, Branch::Task, Branch::UpdateCallback>& callback) {
        return addCallbackToBranchAt(std::get<0>(callback), std::get<1>(callback), std::get<2>(callback));
    });
    _leafCallbacksToRegister.remove_if([this](const std::pair<std::string, Leaf::UpdateCallback>& callback) { return addCallbackToLeafAt(callback.first, callback.second); });
}

} // namespace Tree

} // namespace Splash
