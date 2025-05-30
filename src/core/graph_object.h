/*
 * Copyright (C) 2018 Splash authors
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @graph_object.h
 * Base type for all objects inside root objects
 */

#ifndef SPLASH_GRAPH_OBJECT_H
#define SPLASH_GRAPH_OBJECT_H

#include "./core/base_object.h"
#include "./graphics/api/renderer.h"
#include "./utils/dense_set.h"
#include "./utils/scope_guard.h"

/**
 * DebugGraphicsScope
 * Set the current graphics debug message callback data,
 * and returns it back to its previous value when exiting the scope
 */
#ifdef DEBUGGL
#define DebugGraphicsScope                                                                                                                                                         \
    if (_renderer)                                                                                                                                                                 \
    {                                                                                                                                                                              \
        _renderer->pushRendererMsgCallbackData(getRendererMsgCallbackDataPtr());                                                                                                   \
        OnScopeExit                                                                                                                                                                \
        {                                                                                                                                                                          \
            _renderer->popRendererMsgCallbackData();                                                                                                                               \
        };                                                                                                                                                                         \
    }
#else
#define DebugGraphicsScope                                                                                                                                                         \
    {                                                                                                                                                                              \
    }
#endif

namespace Splash
{

class RootObject;
class Scene;

namespace gfx
{
class Renderer;
}

/*************/
class GraphObject : public BaseObject
{
  public:
    enum class Priority
    {
        NO_RENDER = -1,
        MEDIA = 5,
        BLENDING = 10,
        PRE_FILTER = 15,
        FILTER = 20,
        PRE_CAMERA = 25,
        CAMERA = 30,
        POST_CAMERA = 35,
        WARP = 40,
        GUI = 45,
        WINDOW = 50,
        POST_WINDOW = 55
    };

    enum class Category
    {
        MISC,
        FILTER,
        IMAGE,
        MESH,
        TEXTURE
    };

    enum class TreeRegisterStatus : bool
    {
        Registered,
        NotRegistered
    };

  public:
    /**
     * Constructor.
     */
    GraphObject()
        : _root(nullptr)
    {
        registerAttributes();
    }

    /**
     * Constructor. By default the newly created graph object will be registered
     * into the tree, but in cases where this is not the desired behavior it can
     * be disabled.
     *
     * This can be the case for example for graph objects which are used only in
     * the UI, or which are part of other graph objects which take care of their
     * configuration.
     *
     * \param root Specify the root object.
     * \param registerToTree Register the object into the root tree
     */
    explicit GraphObject(RootObject* root, TreeRegisterStatus registerToTree = TreeRegisterStatus::Registered);

    /**
     * Destructor.
     */
    virtual ~GraphObject();

    /**
     * Access the attributes through operator[].
     * \param attr Name of the attribute.
     * \return Returns a reference to the attribute.
     */
    Attribute& operator[](const std::string& attr);

    /**
     * Add a new attribute to this object, and specify a setter, getter and accepted types.
     *
     * It is also possible for accepted values to be from a generated list. In this case,
     * the list must be given by the getter alongside the actual value (which is first).
     * Also only single value types are accepted in this case, not multiple ones.
     *
     * The accepted (generated) values are to be appended to the actual value of the attribute,
     * when calling the get function.
     *
     * \param name Attribute name
     * \param set Set function
     * \param get Get function
     * \param types Vector of char holding the expected parameters for the set function
     * \param generated Set to true if the accepted values must be taken from a generated list
     * \return Return a reference to the created attribute
     */
    Attribute& addAttribute(
        const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types, bool generated = false) override;
    Attribute& addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types) override;
    Attribute& addAttribute(const std::string& name, const std::function<const Values()>& get) override;

    /**
     * Set the description for the given attribute, if it exists
     * \param name Attribute name
     * \param description Attribute description
     */
    void setAttributeDescription(const std::string& name, const std::string& description);

    /**
     * Get the object's root
     * \return Return a pointer to the root
     */
    inline RootObject* getRoot() const { return _root; }

    /**
     * Set the alias name for the object
     * \param alias Alias name
     */
    inline void setAlias(const std::string& alias) { _alias = alias.empty() ? _name : alias; }

    /**
     * Get the alias name for the object
     * \return Return the alias
     */
    inline std::string getAlias() const { return _alias.empty() ? _name : _alias; }

    /**
     * Set the name of the object.
     * \param name name of the object.
     */
    void setName(const std::string& name) override;

    /**
     * Set the remote type of the object. This implies that this object gets data streamed from a World object
     * \param type Remote type
     */
    inline void setRemoteType(const std::string& type)
    {
        _remoteType = type;
        _isConnectedToRemote = true;
    }

    /**
     * Get the remote type of the object.
     * \return Returns the remote type.
     */
    inline std::string getRemoteType() const { return _remoteType; }

    /**
     * Try to link / unlink the given GraphObject to this
     * \param obj Object to link to
     * \return Returns true if the linking succeeded, or if the object was already linked
     */
    virtual bool linkTo(const std::shared_ptr<GraphObject>& obj);

    /**
     * Unlink a given object
     * \param obj Object to unlink from
     */
    virtual void unlinkFrom(const std::shared_ptr<GraphObject>& obj);

    /**
     * Return a vector of the linked objects
     * \return Returns a vector of the linked objects
     */
    const std::vector<std::weak_ptr<GraphObject>> getLinkedObjects() { return _linkedObjects; }

    /**
     * Get the savability for this object
     * \return Returns true if the object should be saved
     */
    inline bool getSavable() { return _savable; }

    /**
     * Check whether the object's buffer was updated and needs to be re-rendered
     * \return Returns true if the object was updated
     */
    inline virtual bool wasUpdated() const { return _updatedParams; }

    /**
     * Reset the "was updated" status, if needed
     */
    inline virtual void setNotUpdated() { _updatedParams = false; }

    /**
     * Set the object savability
     * \param savable Desired savability
     */
    inline virtual void setSavable(bool savable) { _savable = savable; }

    /**
     * Update the content of the object
     */
    virtual void update() {}

    /**
     * Get the rendering priority for this object
     * The priorities have the following values:
     * - negative value: object not rendered
     * - 10 to 19: pre-cameras rendering
     * - 20 to 29: cameras rendering
     * - 30 to 39: post-cameras rendering
     * - 40 to 49: windows rendering
     * \return Return the rendering priority
     */
    Priority getRenderingPriority() const { return (Priority)((int)_renderingPriority + _priorityShift); }

    /**
     * Set the object's category
     * \param category Category
     */
    void setCategory(Category cat) { _category = cat; }

    /**
     * Get the object's category
     * \return Return the category
     */
    Category getCategory() const { return _category; }

    /**
     * Get the timestamp for the last update of this graph object, or 0
     * \return Return the timestamp in us
     */
    virtual int64_t getTimestamp() const { return 0; }

    /**
     * Set the timestamp
     * \param timestamp Timestamp, in us
     */
    virtual void setTimestamp(int64_t) {}

    /**
     * Set the rendering priority for this object
     * Set GraphObject::getRenderingPriority() for precision about priority
     * \param int Desired priority
     * \return Return true if the priority was set
     */
    bool setRenderingPriority(Priority priority);

    /**
     * Virtual method to render the object
     */
    virtual void render() {}

  protected:
    Category _category{Category::MISC};   //!< Object category, updated by the factory
    std::string _remoteType{""};          //!< When the object root is a Scene, this is the type of the corresponding object in the World
    std::string _alias{""};               //!< Alias name
    std::vector<GraphObject*> _parents{}; //!< Objects parents
    DenseSet<std::string> _lockedAttributes;
    std::unordered_map<std::string, int> _treeCallbackIds{};

    gfx::Renderer::RendererMsgCallbackData _rendererMsgCallbackData;

    Priority _renderingPriority{Priority::NO_RENDER}; //!< Rendering priority, if negative the object won't be rendered
    bool _savable{true};                              //!< True if the object should be saved
    int _priorityShift{0};                            //!< Shift applied to rendering priority

    bool _isConnectedToRemote{false}; //!< True if the object gets data from a World object

    RootObject* _root;                  //!< Root object, Scene or World
    Scene* _scene;                      //!< Pointer to the Scene, if any
    gfx::Renderer* _renderer;           //!< Pointer to the graphic renderer
    TreeRegisterStatus _registerToTree; //!< Register the object to the root tree if true

    std::vector<std::weak_ptr<GraphObject>> _linkedObjects; //!< Linked objects

    /**
     * Get a pointer to the rendering callback data, to be used by the rendering API error callback
     * \return Return a pointer to the rendering callback data
     */
    virtual const gfx::Renderer::RendererMsgCallbackData* getRendererMsgCallbackDataPtr();

    /**
     * Linking method to be defined by derived types
     * \param obj Object to link to
     */
    virtual bool linkIt(const std::shared_ptr<GraphObject>&) { return false; }

    /**
     * Unlinking method to be defined by derived types
     * \param obj Object to unlink from
     */
    virtual void unlinkIt(const std::shared_ptr<GraphObject>&) {}

    /**
     * Inform that the given object is a parent
     * \param obj Parent object
     */
    void linkToParent(GraphObject* obj);

    /**
     * Remove the given object as a parent
     * \param obj Parent object
     */
    void unlinkFromParent(GraphObject* obj);

    /**
     * Register new attributes
     */
    void registerAttributes();

    /**
     * Initialize the tree
     * This is called at object creation, or during setName
     */
    void initializeTree();

    /**
     * Uninitialize the tree
     * This is called at object destruction
     */
    virtual void uninitializeTree();
};

} // namespace Splash

#endif // SPLASH_GRAPH_OBJECT_H
