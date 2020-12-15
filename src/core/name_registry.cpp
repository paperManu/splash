#include "./core/name_registry.h"

#include <algorithm>
#include <iostream>

#include "./core/root_object.h"

namespace Splash
{

std::atomic_uint NameRegistry::_counter{1};

/*************/
std::string NameRegistry::generateName(const std::string& prefix)
{
    while (true)
    {
        auto newName = prefix + "_" + std::to_string(_counter.fetch_add(1));

        if (prefix.empty())
            newName = "unknown_" + newName;

        if (registerName(newName))
        {
            std::lock_guard<std::mutex> lock(_registryMutex);
            _registry.insert(newName);
            return newName;
        }
    }
}

/*************/
bool NameRegistry::registerName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_registryMutex);
    if (_registry.find(name) == _registry.end())
    {
        _registry.insert(name);
        return true;
    }
    else
    {
        return false;
    }
}

/*************/
void NameRegistry::unregisterName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_registryMutex);
    auto nameIt = _registry.find(name);
    if (nameIt != _registry.end())
        _registry.erase(nameIt);
}

} // namespace Splash
