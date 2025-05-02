#include "./controller/controller.h"

#include <algorithm>

#include "./core/factory.h"
#include "./core/scene.h"
#include "core/constants.h"

namespace Splash
{

/*************/
bool ControllerObject::checkObjectExists(const std::string& name) const
{
    const auto objects = getObjectList();
    if (std::find(objects.cbegin(), objects.cend(), name) != objects.cend())
        return true;
    return false;
}

/*************/
std::vector<std::string> ControllerObject::getGeneratedAttributes(const std::string& objName) const
{
    std::vector<std::string> generatedAttributes;
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects/" + objName;
        if (!tree->hasBranchAt(objectPath))
            continue;
        const auto docPath = objectPath + "/documentation";
        const auto attrList = tree->getBranchListAt(docPath);
        for (const auto& attrName : attrList)
        {
            Value value;
            tree->getValueForLeafAt(docPath + "/" + attrName + "/generated", value);
            if (value.as<bool>())
                generatedAttributes.push_back(attrName);
        }
    }

    return generatedAttributes;
}

/*************/
std::shared_ptr<GraphObject> ControllerObject::getObjectPtr(const std::string& name) const
{
    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return {nullptr};

    return scene->getObject(name);
}

/*************/
std::vector<std::shared_ptr<GraphObject>> ControllerObject::getObjectsPtr(const std::vector<std::string>& names) const
{
    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return {};

    std::vector<std::shared_ptr<GraphObject>> objectList;
    for (const auto& name : names)
    {
        auto object = scene->getObject(name);
        if (object)
            objectList.push_back(object);
    }

    return objectList;
}

/*************/
std::string ControllerObject::getObjectAlias(const std::string& name) const
{
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto path = "/" + rootName + "/objects/" + name + "/attributes/alias";
        if (!tree->hasLeafAt(path))
            continue;
        Value value;
        tree->getValueForLeafAt(path, value);
        return value.size() == 0 ? name : value[0].as<std::string>();
    }

    return {};
}

/*************/
std::unordered_map<std::string, std::string> ControllerObject::getObjectAliases() const
{
    auto aliases = std::unordered_map<std::string, std::string>();
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects";
        const auto objectList = tree->getBranchListAt(objectPath);

        for (const auto& objectName : objectList)
        {
            if (aliases.find(objectName) == aliases.end())
            {
                Value value;
                tree->getValueForLeafAt(objectPath + "/" + objectName + "/attributes/alias", value);
                aliases[objectName] = value.size() == 0 ? objectName : value[0].as<std::string>();
            }
        }
    }

    return aliases;
}

/*************/
std::vector<std::string> ControllerObject::getObjectList() const
{
    auto names = std::vector<std::string>();
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects";
        const auto objectList = tree->getBranchListAt(objectPath);

        for (const auto& objectName : objectList)
            if (std::find(names.begin(), names.end(), objectName) == names.end())
                names.push_back(objectName);
    }

    return names;
}

/*************/
Values ControllerObject::getObjectAttributeDescription(const std::string& name, const std::string& attr) const
{
    const auto tree = _root->getTree();
    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects/" + name;
        if (!tree->hasBranchAt(objectPath))
            continue;
        const auto docPath = objectPath + "/documentation/" + attr + "/description";
        if (!tree->hasLeafAt(docPath))
            continue;
        Value value;
        tree->getValueForLeafAt(docPath, value);
        return value.as<Values>();
    }

    return {};
}

/*************/
Values ControllerObject::getObjectAttribute(const std::string& name, const std::string& attr) const
{
    const auto tree = _root->getTree();
    for (const auto& rootName : tree->getBranchList())
    {
        Value value;
        const auto attrPath = "/" + rootName + "/objects/" + name + "/attributes/" + attr;
        if (!tree->hasLeafAt(attrPath))
            continue;
        tree->getValueForLeafAt(attrPath, value);
        return value.as<Values>();
    }

    return {};
}

/*************/
std::unordered_map<std::string, Values> ControllerObject::getObjectAttributes(const std::string& name) const
{
    auto attributes = std::unordered_map<std::string, Values>();
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects/" + name;
        if (!tree->hasBranchAt(objectPath))
            continue;
        const auto attrPath = objectPath + "/attributes";
        const auto attrList = tree->getLeafListAt(attrPath);
        for (const auto& attrName : attrList)
        {
            Value value;
            tree->getValueForLeafAt(attrPath + "/" + attrName, value);
            attributes[attrName] = value.as<Values>();
        }
    }

    return attributes;
}

/*************/
std::unordered_map<std::string, std::vector<std::string>> ControllerObject::getObjectLinks() const
{
    auto links = std::unordered_map<std::string, std::vector<std::string>>();
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects";
        const auto objectList = tree->getBranchListAt(objectPath);

        for (const auto& objectName : objectList)
        {
            const auto linksIt = links.find(objectName);
            if (linksIt == links.end())
                links.emplace(make_pair(objectName, std::vector<std::string>()));

            const auto childrenPath = objectPath + "/" + objectName + "/links/children";

            Value value;
            tree->getValueForLeafAt(childrenPath, value);
            const auto children = value.as<Values>();
            for (const auto& child : children)
            {
                auto& childList = links[objectName];
                const auto childName = child.as<std::string>();
                if (std::find(childList.begin(), childList.end(), childName) == childList.end())
                    childList.push_back(childName);
            }
        }
    }

    return links;
}

/*************/
std::unordered_map<std::string, std::vector<std::string>> ControllerObject::getObjectReversedLinks() const
{
    auto links = std::unordered_map<std::string, std::vector<std::string>>();
    const auto tree = _root->getTree();

    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectPath = "/" + rootName + "/objects";
        const auto objectList = tree->getBranchListAt(objectPath);

        for (const auto& objectName : objectList)
        {
            const auto linksIt = links.find(objectName);
            if (linksIt == links.end())
                links.emplace(make_pair(objectName, std::vector<std::string>()));

            const auto parentsPath = objectPath + "/" + objectName + "/links/parents";

            Value value;
            tree->getValueForLeafAt(parentsPath, value);
            const auto parents = value.as<Values>();
            for (const auto& parent : parents)
            {
                auto& parentList = links[objectName];
                const auto parentName = parent.as<std::string>();
                if (std::find(parentList.begin(), parentList.end(), parentName) == parentList.end())
                    parentList.push_back(parentName);
            }
        }
    }

    return links;
}

/*************/
std::string ControllerObject::getShortDescription(const std::string& type) const
{
    Factory factory;
    return factory.getShortDescription(type);
}

/*************/
std::string ControllerObject::getDescription(const std::string& type) const
{
    Factory factory;
    return factory.getDescription(type);
}

/*************/
std::vector<std::string> ControllerObject::getTypesFromCategory(const GraphObject::Category& category) const
{
    Factory factory;
    return factory.getObjectsOfCategory(category);
}

/*************/
std::map<std::string, std::string> ControllerObject::getObjectTypes() const
{
    auto types = std::map<std::string, std::string>();
    const auto tree = _root->getTree();

    const auto feedListFunc = [&](const std::string& branch) {
        const auto objectPath = "/" + branch + "/objects";
        const auto objectList = tree->getBranchListAt(objectPath);
        for (const auto& objectName : objectList)
        {
            const auto typePath = objectPath + "/" + objectName + "/type";
            assert(tree->hasLeafAt(typePath));
            Value value;
            tree->getValueForLeafAt(typePath, value);
            assert(value.getType() == Value::string);
            const auto type = value[0].as<std::string>();
            types[objectName] = type;
        }
    };

    // Loop over all Scenes
    for (const auto& rootName : tree->getBranchList())
    {
        if (rootName == "world")
            continue;
        feedListFunc(rootName);
    }

    // Loop over the World to get the remote types
    feedListFunc("world");

    return types;
}

/*************/
std::vector<std::string> ControllerObject::getObjectsOfType(const std::string& type) const
{
    std::vector<std::string> objectList;

    const auto tree = _root->getTree();
    for (const auto& rootName : tree->getBranchList())
    {
        const auto objectsPath = "/" + rootName + "/objects";
        for (const auto& objectName : tree->getBranchListAt(objectsPath))
        {
            if (type.empty())
                objectList.push_back(objectName);

            const auto typePath = objectsPath + "/" + objectName + "/type";
            assert(tree->hasLeafAt(typePath));
            Value typeValue;
            tree->getValueForLeafAt(typePath, typeValue);
            if (typeValue[0].as<std::string>() == type)
                objectList.push_back(objectName);
        }
    }

    std::sort(objectList.begin(), objectList.end());
    objectList.erase(std::unique(objectList.begin(), objectList.end()), objectList.end());

    return objectList;
}

/*************/
void ControllerObject::sendBuffer(SerializedObject&& buffer) const
{
    if (_root)
        _root->sendBuffer(std::move(buffer));
}

/*************/
void ControllerObject::setWorldAttribute(const std::string& name, const Values& values) const
{
    const auto tree = _root->getTree();
    const auto attrPath = "/world/attributes/" + name;
    if (tree->hasLeafAt(attrPath))
        tree->setValueForLeafAt(attrPath, values);
    else
        _root->addTreeCommand("world", RootObject::Command::callRoot, {name, values});
}

/*************/
void ControllerObject::setInScene(const std::string& name, const Values& values) const
{
    const auto tree = _root->getTree();
    const auto attrPath = "/" + _root->getName() + "/attributes/" + name;
    if (tree->hasLeafAt(attrPath))
        tree->setValueForLeafAt(attrPath, values);
    else
        _root->addTreeCommand(_root->getName(), RootObject::Command::callRoot, {name, values});
}

/*************/
Values ControllerObject::getWorldAttribute(const std::string& attr) const
{
    const auto tree = _root->getTree();
    const auto attrPath = "/world/attributes/" + attr;
    if (!tree->hasLeafAt(attrPath))
        return {};
    Value value;
    tree->getValueForLeafAt(attrPath, value);
    return value.as<Values>();
}

/*************/
void ControllerObject::setObjectAttribute(const std::string& name, const std::string& attr, const Values& values) const
{
    if (name.empty())
        return;

    const auto tree = _root->getTree();
    const auto branchList = tree->getBranchList();
    for (const auto& branchName : branchList)
    {
        const auto path = "/" + branchName + "/objects/" + name + "/attributes/" + attr;
        if (tree->hasLeafAt(path))
            tree->setValueForLeafAt(path, values, 0, true);
        else
            _root->addTreeCommand(branchName, RootObject::Command::callObject, {name, attr, values});
    }
}

/*************/
void ControllerObject::setObjectsOfType(const std::string& type, const std::string& attr, const Values& values) const
{
    const auto tree = _root->getTree();
    const auto branchList = tree->getBranchList();
    for (const auto& branchName : branchList)
    {
        const auto path = "/" + branchName + "/objects";
        if (!tree->hasBranchAt(path))
            continue;
        const auto objectList = tree->getBranchListAt(path);
        for (const auto& objectName : objectList)
        {
            const auto typePath = path + "/" + objectName + "/type";
            Value objectType;
            if (!tree->getValueForLeafAt(typePath, objectType))
                continue;
            if (objectType[0].as<std::string>() == type)
            {
                const auto attrPath = path + "/" + objectName + "/attributes/" + attr;
                if (tree->hasLeafAt(attrPath))
                    tree->setValueForLeafAt(attrPath, values);
                else
                    _root->addTreeCommand(branchName, RootObject::Command::callObject, {objectName, attr, values});
            }
        }
    }
}

/*************/
void ControllerObject::setUserInputCallback(const UserInput::State& state, std::function<void(const UserInput::State&)>& cb) const
{
    UserInput::setCallback(state, cb);
}

} // namespace Splash
