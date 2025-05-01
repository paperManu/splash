#include "./core/graph_object.h"

#include <algorithm>

#include "./core/root_object.h"
#include "./core/scene.h"

namespace Splash
{

/*************/
GraphObject::GraphObject(RootObject* root, TreeRegisterStatus registerToTree)
    : _root(root)
    , _registerToTree(registerToTree)
{
    initializeTree();
    registerAttributes();

    _scene = dynamic_cast<Scene*>(root);
    if (_scene != nullptr)
        _renderer = _scene->getRenderer();
    else
        _renderer = nullptr;
}

/*************/
GraphObject::~GraphObject()
{
    uninitializeTree();
}

/*************/
Attribute& GraphObject::operator[](const std::string& attr)
{
    std::unique_lock<std::recursive_mutex> lock(_attribMutex);
    auto attribFunction = _attribFunctions.find(attr);
    return attribFunction->second;
}

/*************/
Attribute& GraphObject::addAttribute(
    const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types, bool generated)
{
    auto& attribute = BaseObject::addAttribute(name, set, get, types, generated);
    initializeTree();
    return attribute;
}

/*************/
Attribute& GraphObject::addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types)
{
    return BaseObject::addAttribute(name, set, types);
}

/*************/
Attribute& GraphObject::addAttribute(const std::string& name, const std::function<const Values()>& get)
{
    auto& attribute = BaseObject::addAttribute(name, get);
    initializeTree();
    return attribute;
}

/*************/
void GraphObject::setAttributeDescription(const std::string& name, const std::string& description)
{
    BaseObject::setAttributeDescription(name, description);
    initializeTree();
}

/*************/
void GraphObject::linkToParent(GraphObject* obj)
{
    auto parentIt = find(_parents.begin(), _parents.end(), obj);
    if (parentIt == _parents.end())
        _parents.push_back(obj);

    // If the object is not registered to the tree, we don't update the link status there
    if (_registerToTree == TreeRegisterStatus::NotRegistered)
        return;

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/parents";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto parents = value.as<Values>();
        if (find_if(parents.begin(), parents.end(), [&](const Value& a) { return a.as<std::string>() == obj->getName(); }) == parents.end())
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

    // If the object is not registered to the tree, we don't update the link status there
    if (_registerToTree == TreeRegisterStatus::NotRegistered)
        return;

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/parents";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto parents = value.as<Values>();
        parents.erase(remove_if(parents.begin(), parents.end(), [&](const Value& a) { return a.as<std::string>() == obj->getName(); }), parents.end());
        tree->setValueForLeafAt(path, parents);
    }
}

/*************/
bool GraphObject::linkTo(const std::shared_ptr<GraphObject>& obj)
{
    if (obj.get() == this)
        return false;

    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const std::weak_ptr<GraphObject>& o) {
        auto object = o.lock();
        if (!object)
            return false;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt != _linkedObjects.end())
        return true;

    if (!linkIt(obj))
    {
        Log::get() << Log::WARNING << "GraphObject::" << __FUNCTION__ << " - Unable to link objects of type " << _type << " and " << obj->getType() << Log::endl;
        return false;
    }

    _linkedObjects.push_back(obj);
    obj->linkToParent(this);

    // If the object is not registered to the tree, we don't update the link status there
    if (_registerToTree == TreeRegisterStatus::NotRegistered)
        return true;

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/children";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto children = value.as<Values>();
        if (find_if(children.begin(), children.end(), [&](const Value& a) { return a.as<std::string>() == obj->getName(); }) == children.end())
        {
            children.emplace_back(obj->getName());
            tree->setValueForLeafAt(path, children);
        }
    }

    return true;
}

/*************/
void GraphObject::unlinkFrom(const std::shared_ptr<GraphObject>& obj)
{
    auto objectIt = find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const std::weak_ptr<GraphObject>& o) {
        auto object = o.lock();
        if (!object)
            return true;
        if (object == obj)
            return true;
        return false;
    });

    if (objectIt == _linkedObjects.end())
        return;

    unlinkIt(obj);
    _linkedObjects.erase(objectIt);
    obj->unlinkFromParent(this);

    // If the object is not registered to the tree, we don't update the link status there
    if (_registerToTree == TreeRegisterStatus::NotRegistered)
        return;

    if (_root && !_name.empty() && !obj->getName().empty())
    {
        auto rootName = _root->getName();
        auto tree = _root->getTree();
        auto path = "/" + rootName + "/objects/" + _name + "/links/children";
        assert(tree->hasLeafAt(path));

        Value value;
        tree->getValueForLeafAt(path, value);
        auto children = value.as<Values>();
        children.erase(remove_if(children.begin(), children.end(), [&](const Value& a) { return a.as<std::string>() == obj->getName(); }), children.end());
        tree->setValueForLeafAt(path, children);
    }
}

/*************/
void GraphObject::setName(const std::string& name)
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
    addAttribute(
        "alias",
        [&](const Values& args) {
            auto alias = args[0].as<std::string>();
            setAlias(alias);
            return true;
        },
        [&]() -> Values { return {getAlias()}; },
        {'s'});
    setAttributeDescription("alias", "Alias name");

    addAttribute(
        "savable",
        [&](const Values& args) {
            auto savable = args[0].as<bool>();
            setSavable(savable);
            return true;
        },
        [&]() -> Values { return {_savable}; },
        {'b'});
    setAttributeDescription("savable", "If true, the object will be saved in the configuration file. This should NOT be modified by hand");

    addAttribute(
        "priorityShift",
        [&](const Values& args) {
            _priorityShift = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_priorityShift}; },
        {'i'});
    setAttributeDescription("priorityShift",
        "Shift to the default rendering priority value, for those cases where two objects should be rendered in a specific order. Higher value means lower priority");

    addAttribute("switchLock",
        [&](const Values& args) {
            std::unique_lock<std::recursive_mutex> lock(_attribMutex);
            const auto attribName = args[0].as<std::string>();
            auto attribIterator = _attribFunctions.find(attribName);
            if (attribIterator == _attribFunctions.end())
                return false;

            std::string status;
            auto& attribFunctor = attribIterator->second;
            if (attribFunctor.isLocked())
            {
                status = "Unlocked";
                attribFunctor.unlock();
                _lockedAttributes.erase(attribName);
            }
            else
            {
                status = "Locked";
                attribFunctor.lock();
                _lockedAttributes.insert(attribName);
            }

            return true;
        },
        {'s'});

    addAttribute("lockedAttributes", [&]() -> Values {
        std::unique_lock<std::recursive_mutex> lock(_attribMutex);
        Values lockedAttributes;
        for (const auto& attr : _lockedAttributes)
            lockedAttributes.push_back(Value(attr));
        return lockedAttributes;
    });

    addAttribute(
        "timestamp", [](const Values&) { return true; }, [&]() -> Values { return {getTimestamp()}; }, {'i'});
    setAttributeDescription("timestamp", "Timestamp (in µs) for the current buffer, based on the latest image data created/received");
}

/*************/
void GraphObject::initializeTree()
{
    if (!_root || _name.empty())
        return;

    if (_registerToTree == TreeRegisterStatus::NotRegistered)
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
        std::lock_guard<std::recursive_mutex> lock(_attribMutex);
        for (const auto& attribute : _attribFunctions)
        {
            if (!attribute.second.hasGetter())
                continue;
            auto attributeName = attribute.first;
            auto leafPath = attrPath + attributeName;
            if (tree->hasLeafAt(leafPath))
                continue;

            tree->createLeafAt(leafPath);
            _treeCallbackIds[attributeName] = tree->addCallbackToLeafAt(leafPath, [=, this](const Value& value, const std::chrono::system_clock::time_point& /*timestamp*/) {
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
        const auto docPath = path + "/documentation/";
        const auto attributesDescriptions = getAttributesDescriptions();
        for (const auto& d : attributesDescriptions)
        {
            const auto attrName = d[0].as<std::string>();
            const auto attrPath = docPath + attrName;
            tree->createBranchAt(attrPath);
            tree->createLeafAt(attrPath + "/description");
            tree->createLeafAt(attrPath + "/arguments");
            tree->createLeafAt(attrPath + "/generated");

            const auto description = d[1].as<std::string>();
            const auto arguments = d[2].as<Values>();
            const auto generated = d[3].as<bool>();
            tree->setValueForLeafAt(attrPath + "/description", description);
            tree->setValueForLeafAt(attrPath + "/arguments", arguments);
            tree->setValueForLeafAt(attrPath + "/generated", generated);
        }
    }

    // Remove leaves for attributes which do not exist anymore
    std::lock_guard<std::recursive_mutex> lock(_attribMutex);
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

/*************/
const gfx::Renderer::RendererMsgCallbackData* GraphObject::getRendererMsgCallbackDataPtr()
{
    _rendererMsgCallbackData.name = _name;
    _rendererMsgCallbackData.type = _type;
    return &_rendererMsgCallbackData;
}

} // namespace Splash
