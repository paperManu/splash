#include "./core/graph_object.h"

#include <algorithm>

#include "./core/root_object.h"

using namespace std;

namespace Splash
{

/*************/
GraphObject::GraphObject(RootObject* root)
    : _root(root)
{
    initializeTree();
    registerAttributes();
}

/*************/
GraphObject::~GraphObject()
{
    uninitializeTree();
}

/*************/
Attribute& GraphObject::operator[](const string& attr)
{
    auto attribFunction = _attribFunctions.find(attr);
    return attribFunction->second;
}

/*************/
Attribute& GraphObject::addAttribute(const string& name, const function<bool(const Values&)>& set, const vector<char>& types)
{
    return BaseObject::addAttribute(name, set, types);
}

/*************/
Attribute& GraphObject::addAttribute(const string& name, const function<bool(const Values&)>& set, const function<const Values()>& get, const vector<char>& types)
{
    auto& attribute = BaseObject::addAttribute(name, set, get, types);
    initializeTree();
    return attribute;
}

/*************/
void GraphObject::linkToParent(GraphObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt == _parents.end())
        _parents.push_back(obj);

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/parents";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto parents = value.as<Values>();
        if (find_if(parents.begin(), parents.end(), [&](const Value& a) { return a.as<string>() == obj->getName(); }) == parents.end())
        {
            parents.emplace_back(obj->getName());
            tree->setValueForLeafAt(path, parents);
        }
    }
}

/*************/
void GraphObject::unlinkFromParent(GraphObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt != _parents.end())
        _parents.erase(parentIt);

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/parents";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto parents = value.as<Values>();
        parents.erase(remove_if(parents.begin(), parents.end(), [&](const Value& a) { return a.as<string>() == obj->getName(); }), parents.end());
        tree->setValueForLeafAt(path, parents);
    }
}

/*************/
bool GraphObject::linkTo(const shared_ptr<GraphObject>& obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const weak_ptr<GraphObject>& o) {
        auto object = o.lock();
        if (!object)
            return false;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt != _linkedObjects.end())
        return false;

    _linkedObjects.push_back(obj);
    obj->linkToParent(this);

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/children";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto children = value.as<Values>();
        if (find_if(children.begin(), children.end(), [&](const Value& a) { return a.as<string>() == obj->getName(); }) == children.end())
        {
            children.emplace_back(obj->getName());
            tree->setValueForLeafAt(path, children);
        }
    }

    return true;
}

/*************/
void GraphObject::unlinkFrom(const shared_ptr<GraphObject>& obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const weak_ptr<GraphObject>& o) {
        auto object = o.lock();
        if (!object)
            return false;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt == _linkedObjects.end())
        return;

    _linkedObjects.erase(objectIt);
    obj->unlinkFromParent(this);

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/children";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto children = value.as<Values>();
        children.erase(remove_if(children.begin(), children.end(), [&](const Value& a) { return a.as<string>() == obj->getName(); }), children.end());
        tree->setValueForLeafAt(path, children);
    }
}

/*************/
void GraphObject::setName(const string& name)
{
    if (name.empty())
        return;

    auto oldName = _name;
    _name = name;

    if (!_root)
        return;

    if (oldName.empty())
    {
        initializeTree();
    }
    else
    {
        auto tree = _root->getTree();
        auto path = "/" + _root->getName() + "/objects/" + oldName;
        if (tree->hasBranchAt(path))
            tree->renameBranchAt(path, name);
    }
}

/*************/
bool GraphObject::setRenderingPriority(Priority priority)
{
    if (priority < Priority::PRE_CAMERA || priority >= Priority::POST_WINDOW)
        return false;
    _renderingPriority = priority;
    return true;
}
/*************/
void GraphObject::registerAttributes()
{
    addAttribute("alias",
        [&](const Values& args) {
            auto alias = args[0].as<string>();
            setAlias(alias);
            return true;
        },
        [&]() -> Values { return {getAlias()}; },
        {'s'});
    setAttributeDescription("alias", "Alias name");

    addAttribute("savable",
        [&](const Values& args) {
            auto savable = args[0].as<bool>();
            setSavable(savable);
            return true;
        },
        [&]() -> Values { return {_savable}; },
        {'n'});
    setAttributeDescription("savable", "If true, the object will be saved in the configuration file. This should NOT be modified by hand");

    addAttribute("priorityShift",
        [&](const Values& args) {
            _priorityShift = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_priorityShift}; },
        {'n'});
    setAttributeDescription("priorityShift",
        "Shift to the default rendering priority value, for those cases where two objects should be rendered in a specific order. Higher value means lower priority");

    addAttribute("switchLock",
        [&](const Values& args) {
            auto attribIterator = _attribFunctions.find(args[0].as<string>());
            if (attribIterator == _attribFunctions.end())
                return false;

            string status;
            auto& attribFunctor = attribIterator->second;
            if (attribFunctor.isLocked())
            {
                status = "Unlocked";
                attribFunctor.unlock();
            }
            else
            {
                status = "Locked";
                attribFunctor.lock();
            }

            Log::get() << Log::MESSAGE << _name << "~~" << args[0].as<string>() << " - " << status << Log::endl;
            return true;
        },
        {'s'});
}

/*************/
void GraphObject::initializeTree()
{
    if (!_root || _name.empty())
        return;

    auto tree = _root->getTree();
    auto path = "/" + _root->getName() + "/objects/" + _name;
    if (!tree->hasBranchAt(path))
        tree->createBranchAt(path);
    if (!tree->hasBranchAt(path + "/attributes"))
        tree->createBranchAt(path + "/attributes");
    if (!tree->hasBranchAt(path + "/attributes"))
        tree->createBranchAt(path + "/attributes");
    if (!tree->hasBranchAt(path + "/documentation"))
        tree->createBranchAt(path + "/documentation");
    if (!tree->hasBranchAt(path + "/links"))
    {
        tree->createBranchAt(path + "/links");
        tree->createLeafAt(path + "/links/children");
        tree->createLeafAt(path + "/links/parents");
    }

    if (!tree->hasLeafAt(path + "/type"))
        tree->createLeafAt(path + "/type");
    tree->setValueForLeafAt(path + "/type", _type);

    // Create the leaves for the attributes in the tree
    {
        auto attrPath = path + "/attributes/";
        for (const auto& attribute : _attribFunctions)
        {
            if (!attribute.second.hasGetter())
                continue;
            auto attributeName = attribute.first;
            auto leafPath = attrPath + attributeName;
            if (tree->hasLeafAt(leafPath))
                continue;

            tree->createLeafAt(leafPath);
            _treeCallbackIds[attributeName] = tree->addCallbackToLeafAt(leafPath, [=](const Value& value, const chrono::system_clock::time_point& /*timestamp*/) {
                auto attribIt = _attribFunctions.find(attributeName);
                if (attribIt == _attribFunctions.end())
                    return;
                if (value == attribIt->second())
                    return;
                setAttribute(attributeName, value.as<Values>());
            });
        }
    }

    // Store the documentation inside the tree
    {
        auto docPath = path + "/documentation/";
        auto attributesDescriptions = getAttributesDescriptions();
        for (auto& d : attributesDescriptions)
        {
            if (d[1].size() == 0)
                continue;
            if (d[2].size() == 0)
                continue;
            auto attrName = d[0].as<string>();
            auto attrPath = docPath + attrName;
            if (tree->hasBranchAt(attrPath))
                continue;
            tree->createBranchAt(attrPath);
            tree->createLeafAt(attrPath + "/description");
            tree->createLeafAt(attrPath + "/arguments");

            auto description = d[1].as<string>();
            auto arguments = d[2].as<Values>();
            tree->setValueForLeafAt(attrPath + "/description", description);
            tree->setValueForLeafAt(attrPath + "/arguments", arguments);
        }
    }

    // Remove leaves for attributes which do not exist anymore
    for (const auto& leafName : tree->getLeafListAt(path + "/attributes"))
    {
        if (_attribFunctions.find(leafName) != _attribFunctions.end())
            continue;
        tree->removeLeafAt(path + "/attributes/" + leafName);
    }
}

/*************/
void GraphObject::uninitializeTree()
{
    if (!_root || _name.empty())
        return;

    auto tree = _root->getTree();
    auto path = "/" + _root->getName() + "/objects/" + _name;
    if (tree->hasBranchAt(path))
        tree->removeBranchAt(path);
}

} // namespace Splash
