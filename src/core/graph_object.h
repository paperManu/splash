/*
 * Copyright (C) 2018 Emmanuel Durand
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

namespace Splash
{

class RootObject;

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
        IMAGE,
        MESH,
        TEXTURE
    };

  public:
    /**
     * \brief Constructor.
     */
    GraphObject()
        : _root(nullptr)
    {
        registerAttributes();
    }

    /**
     * \brief Constructor.
     * \param root Specify the root object.
     */
    explicit GraphObject(RootObject* root);

    /**
     * \brief Destructor.
     */
    virtual ~GraphObject();

    /**
     * \brief Safe bool idiom.
     */
    virtual explicit operator bool() const { return true; }

    /**
     * \brief Access the attributes through operator[].
     * \param attr Name of the attribute.
     * \return Returns a reference to the attribute.
     */
    Attribute& operator[](const std::string& attr);

    /**
     * \brief Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    Attribute& addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types = {}) override;

    /**
     * \brief Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param get Get function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    Attribute& addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types = {}) override;

    /**
     * \brief Get the real type of this BaseObject, as a std::string.
     * \return Returns the type.
     */
    inline std::string getType() const { return _type; }

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
     * \brief Set the name of the object.
     * \param name name of the object.
     */
    void setName(const std::string& name) override;

    /**
     * \brief Set the remote type of the object. This implies that this object gets data streamed from a World object
     * \param type Remote type
     */
    inline void setRemoteType(const std::string& type)
    {
        _remoteType = type;
        _isConnectedToRemote = true;
    }

    /**
     * \brief Get the remote type of the object.
     * \return Returns the remote type.
     */
    inline std::string getRemoteType() const { return _remoteType; }

    /**
     * \brief Try to link / unlink the given GraphObject to this
     * \param obj Object to link to
     * \return Returns true if the linking succeeded
     */
    virtual bool linkTo(const std::shared_ptr<GraphObject>& obj);

    /**
     * \brief Unlink a given object
     * \param obj Object to unlink from
     */
    virtual void unlinkFrom(const std::shared_ptr<GraphObject>& obj);

    /**
     * \brief Return a vector of the linked objects
     * \return Returns a vector of the linked objects
     */
    const std::vector<std::weak_ptr<GraphObject>> getLinkedObjects() { return _linkedObjects; }

    /**
     * \brief Get the savability for this object
     * \return Returns true if the object should be saved
     */
    inline bool getSavable() { return _savable; }

    /**
     * \brief Check whether the object's buffer was updated and needs to be re-rendered
     * \return Returns true if the object was updated
     */
    inline virtual bool wasUpdated() const { return _updatedParams; }

    /**
     * \brief Reset the "was updated" status, if needed
     */
    inline virtual void setNotUpdated() { _updatedParams = false; }

    /**
     * \brief Set the object savability
     * \param savable Desired savability
     */
    inline virtual void setSavable(bool savable) { _savable = savable; }

    /**
     * \brief Update the content of the object
     */
    virtual void update() {}

    /**
     * \brief Get the rendering priority for this object
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
     * \brief Set the rendering priority for this object
     * Set GraphObject::getRenderingPriority() for precision about priority
     * \param int Desired priority
     * \return Return true if the priority was set
     */
    bool setRenderingPriority(Priority priority);

    /**
     * \brief Virtual method to render the object
     */
    virtual void render() {}

  public:
    bool _savable{true}; //!< True if the object should be saved

  protected:
    Category _category{Category::MISC};   //!< Object category, updated by the factory
    std::string _type{"baseobject"};      //!< Internal type
    std::string _remoteType{""};          //!< When the object root is a Scene, this is the type of the corresponding object in the World
    std::string _alias{""};               //!< Alias name
    std::vector<GraphObject*> _parents{}; //!< Objects parents
    std::unordered_map<std::string, int> _treeCallbackIds{};

    Priority _renderingPriority{Priority::NO_RENDER}; //!< Rendering priority, if negative the object won't be rendered
    int _priorityShift{0};                            //!< Shift applied to rendering priority

    bool _isConnectedToRemote{false}; //!< True if the object gets data from a World object

    RootObject* _root;                                      //!< Root object, Scene or World
    std::vector<std::weak_ptr<GraphObject>> _linkedObjects; //!< Linked objects

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
     * \brief Register new attributes
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
