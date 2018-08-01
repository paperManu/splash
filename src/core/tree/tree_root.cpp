#include "./core/tree/tree_root.h"

#include "./utils/log.h"

using namespace std;

namespace Splash
{

namespace Tree
{

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
bool Root::addBranchAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return false;
    }

    auto newBranchName = parts.back();
    parts.pop_back();

    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    auto newBranch = make_unique<Branch>(newBranchName);
    if (!holdingBranch->addBranch(move(newBranch)))
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " already has a branch named " << newBranchName << Log::endl;
        return false;
    }

    if (!silent)
    {
        lock_guard<mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::AddBranch, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
bool Root::addLeafAt(const std::string& path, Values value, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return false;
    }

    auto newLeafName = parts.back();
    parts.pop_back();

    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    auto newLeaf = make_unique<Leaf>(newLeafName, value);
    if (!holdingBranch->addLeaf(move(newLeaf)))
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " already has a leaf named " << newLeafName << Log::endl;
        return false;
    }

    if (!silent)
    {
        lock_guard<mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::AddLeaf, Values({path, value}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
void Root::addSeedToQueue(Tree::Task taskType, Values args, chrono::system_clock::time_point timestamp)
{
    auto seed = make_tuple(taskType, args, timestamp, UUID(false));
    lock_guard<mutex> lock(_taskMutex);
    _taskQueue.push_back(seed);
}

/*************/
void Root::addSeedsToQueue(const list<Seed>& seeds)
{
    lock_guard<mutex> lock(_taskMutex);
    for (const auto& seed : seeds)
        _taskQueue.push_back(seed);
}

/*************/
void Root::clearSeedList()
{
    lock_guard<mutex> lock(_updatesMutex);
    _updates.clear();
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
bool Root::getValueForLeafAt(const string& path, Value& value)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return false;
    }

    auto leafName = parts.back();
    parts.pop_back();

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
bool Root::hasLeafAt(const string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return false;
    }

    auto leafName = parts.back();
    parts.pop_back();

    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
        return false;

    if (holdingBranch->hasLeaf(leafName))
        return true;
    else
        return false;
}

/*************/
bool Root::setValueForLeafAt(const string& path, const Values& value, int64_t timestamp, bool silent)
{
    chrono::system_clock::time_point timePoint;
    if (timestamp)
        timePoint = chrono::system_clock::time_point(chrono::milliseconds(timestamp));
    else
        timePoint = chrono::system_clock::now();

    return setValueForLeafAt(path, value, timePoint, silent);
}

/*************/
bool Root::setValueForLeafAt(const string& path, const Values& value, chrono::system_clock::time_point timestamp, bool silent)
{
    auto leaf = getLeafAt(processPath(path));
    if (!leaf)
        return false;

    if (!leaf->set(value, timestamp))
        return false;

    if (!silent)
    {
        lock_guard<mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::SetLeaf, Values({path, value}), timestamp, _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
}

/*************/
list<Seed> Root::getSeedList()
{
    list<Seed> updates;
    lock_guard<mutex> lock(_updatesMutex);
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
    decltype(_taskQueue) tasks;
    swap(tasks, _taskQueue);
    _taskMutex.unlock();

    tasks.sort([](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

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
                Log::get() << Log::WARNING << errorStr << Log::endl;
                throw runtime_error(errorStr);
            }

            auto branchPath = args[0].as<string>();
            if (!addBranchAt(branchPath, true))
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
                Log::get() << Log::WARNING << errorStr << Log::endl;
                throw runtime_error(errorStr);
            }

            auto leafPath = args[0].as<string>();
            auto values = args.size() == 2 ? args[1].as<Values>() : Values();
            if (addLeafAt(leafPath, values, true))
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
                Log::get() << Log::WARNING << errorStr << Log::endl;
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
                Log::get() << Log::WARNING << errorStr << Log::endl;
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
        case Task::SetLeaf:
            if (args.size() < 1)
            {
                auto errorStr = "Root::" + string(__FUNCTION__) + " - Wrong number of arguments for task SetLeaf";
                Log::get() << Log::WARNING << errorStr << Log::endl;
                throw runtime_error(errorStr);
            }
            auto leafPath = args[0].as<string>();
            auto values = args[1].as<Values>();
            if (!setValueForLeafAt(leafPath, values, timestamp, true))
            {
                _hasError = true;
                _errorMsg = "Error while setting leaf at path " + leafPath;
                break;
            }

            break;
        }
    }

    if (propagate)
        _updates.merge(tasks, [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

    return true;
}

/*************/
bool Root::removeBranchAt(const string& path, bool silent)
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return false;
    }

    auto branchToRemove = parts.back();
    parts.pop_back();

    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Could not find branch holding path " << path << Log::endl;
        return false;
    }

    if (!holdingBranch->removeBranch(branchToRemove))
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " does not have a branch named " << branchToRemove << Log::endl;
        return false;
    }

    if (!silent)
    {
        lock_guard<mutex> lock(_updatesMutex);
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
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return false;
    }

    auto leafToRemove = parts.back();
    parts.pop_back();

    auto holdingBranch = getBranchAt(parts);
    if (!holdingBranch)
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Could not find branch holding leaf " << path << Log::endl;
        return false;
    }

    if (!holdingBranch->removeLeaf(leafToRemove))
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << holdingBranch->getPath() << " does not have a leaf named " << leafToRemove << Log::endl;
        return false;
    }

    if (!silent)
    {
        lock_guard<mutex> lock(_updatesMutex);
        auto seed = make_tuple(Task::RemoveLeaf, Values({path}), chrono::system_clock::now(), _uuid);
        _updates.emplace_back(move(seed));
    }

    return true;
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
            Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << branch->getPath() << " does not have branch " << part << Log::endl;
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
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return nullptr;
    }

    return getBranchAt(parts);
}

/*************/
Leaf* Root::getLeafAt(const string& path) const
{
    auto parts = processPath(path);
    if (parts.empty())
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Given path is not valid: " << path << Log::endl;
        return nullptr;
    }

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
            Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << branch->getPath() << " does not have branch " << part << Log::endl;
            return nullptr;
        }
        branch = childBranch;
    }

    auto leaf = branch->getLeaf(leafName);
    if (!leaf)
    {
        Log::get() << Log::WARNING << "Tree::Root::" << __FUNCTION__ << " - Branch " << branch->getPath() << " does not have a leaf name " << leafName << Log::endl;
        return nullptr;
    }

    return leaf;
}

/*************/
list<string> Root::processPath(const string& path)
{
    if (path[0] != '/')
        return {};

    list<string> parts;
    auto subpath = path.substr(1);
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

} // namespace Tree

} // namespace Splash
