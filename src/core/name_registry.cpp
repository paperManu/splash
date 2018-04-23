#include "./core/name_registry.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace Splash
{

std::atomic_uint NameRegistry::_counter{1};

/*************/
std::string NameRegistry::generateName(const string& prefix)
{
    while (true)
    {
        auto newName = prefix + "_" + to_string(_counter.fetch_add(1));

        if (prefix.empty())
            newName = "unknown_" + newName;

        if (registerName(newName))
        {
            lock_guard<mutex> lock(_registryMutex);
            _registry.push_back(newName);
            return newName;
        }
    }
}

/*************/
bool NameRegistry::registerName(const string& name)
{
    lock_guard<mutex> lock(_registryMutex);
    if (std::find(_registry.begin(), _registry.end(), name) == _registry.end())
    {
        _registry.push_back(name);
        return true;
    }
    else
    {
        return false;
    }
}

/*************/
void NameRegistry::unregisterName(const string& name)
{
    lock_guard<mutex> lock(_registryMutex);
    auto nameIt = std::find(_registry.begin(), _registry.end(), name);
    if (nameIt != _registry.end())
        _registry.erase(nameIt);
}

} // namespace Splash
