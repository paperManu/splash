/*
 * Copyright (C) 2017 Emmanuel Durand
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
 * @base_object.h
 * Base type for all objects, from which more complex classes derive
 */

#ifndef SPLASH_BASE_OBJECT_H
#define SPLASH_BASE_OBJECT_H

#include <atomic>
#include <condition_variable>
#include <future>
#include <json/json.h>
#include <list>
#include <map>
#include <unordered_map>

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

class RootObject;

/*************/
class BaseObject : public std::enable_shared_from_this<BaseObject>
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
    BaseObject()
        : _root(nullptr)
    {
        registerAttributes();
    }

    /**
     * \brief Constructor.
     * \param root Specify the root object.
     */
    BaseObject(RootObject* root)
        : _root(root)
    {
        registerAttributes();
    }

    /**
     * \brief Destructor.
     */
    virtual ~BaseObject() {}

    /**
     * \brief Safe bool idiom.
     */
    virtual explicit operator bool() const { return true; }

    /**
     * \brief Access the attributes through operator[].
     * \param attr Name of the attribute.
     * \return Returns a reference to the attribute.
     */
    AttributeFunctor& operator[](const std::string& attr);

    /**
     * \brief Get the real type of this BaseObject, as a std::string.
     * \return Returns the type.
     */
    inline std::string getType() const { return _type; }

    /**
     * \brief Set the ID of the object.
     * \param id ID of the object.
     */
    inline void setId(unsigned long id) { _id = id; }

    /**
     * \brief Get the ID of the object.
     * \return Returns the ID of the object.
     */
    inline unsigned long getId() const { return _id; }

    /**
     * \brief Set the name of the object.
     * \param name name of the object.
     */
    inline virtual std::string setName(const std::string& name)
    {
        _name = name;
        return _name;
    }

    /**
     * \brief Get the name of the object.
     * \return Returns the name of the object.
     */
    inline std::string getName() const { return _name; }

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
     * \brief Try to link / unlink the given BaseObject to this
     * \param obj Object to link to
     * \return Returns true if the linking succeeded
     */
    virtual bool linkTo(const std::shared_ptr<BaseObject>& obj);

    /**
     * \brief Unlink a given object
     * \param obj Object to unlink from
     */
    virtual void unlinkFrom(const std::shared_ptr<BaseObject>& obj);

    /**
     * \brief Return a vector of the linked objects
     * \return Returns a vector of the linked objects
     */
    const std::vector<std::shared_ptr<BaseObject>> getLinkedObjects();

    /**
     * \brief Set the specified attribute
     * \param attrib Attribute name
     * \param args Values object which holds attribute values
     * \return Returns true if the parameter exists and was set
     */
    bool setAttribute(const std::string& attrib, const Values& args = {});

    /**
     * \brief Get the specified attribute
     * \param attrib Attribute name
     * \param args Values object which will hold the attribute values
     * \param includeDistant Return true even if the attribute is distant
     * \param includeNonSavable Return true even if the attribute is not savable
     * \return Return true if the parameter exists and is savable
     */
    bool getAttribute(const std::string& attrib, Values& args, bool includeDistant = false, bool includeNonSavable = false) const;

    /**
     * \brief Get all the savable attributes as a map
     * \param includeDistant Also include the distant attributes
     * \return Return the map of all the attributes
     */
    std::unordered_map<std::string, Values> getAttributes(bool includeDistant = false) const;

    /**
     * \brief Get the map of the attributes which should be updated from World to Scene
     * \brief This is the case when the distant object is different from the World one
     * \return Returns a map of the distant attributes
     */
    std::unordered_map<std::string, Values> getDistantAttributes() const;

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
     * Set the object as a ghost, meaning it mimics an object in another scene
     * \param ghost If true, set as ghost
     */
    inline void setGhost(bool ghost) { _ghost = ghost; }

    /**
     * Get whether the object ghosts an object in another scene
     * \return Return true if this object is a ghost
     */
    inline bool isGhost() const { return _ghost; }

    /**
     * \brief Update the content of the object
     */
    virtual void update() {}

    /**
     * \brief Converts a Value as a Json object
     * \param values Value to convert
     * \param asObject If true, return a Json object
     * \return Returns a Json object
     */
    Json::Value getValuesAsJson(const Values& values, bool asObject = false) const;

    /**
     * \brief Get the object's configuration as a Json object
     * \return Returns a Json object
     */
    Json::Value getConfigurationAsJson() const;

    /**
     * \brief Get the description for the given attribute, if it exists
     * \param name Name of the attribute
     * \return Returns the description for the attribute
     */
    std::string getAttributeDescription(const std::string& name);

    /**
     * \brief Get a Values holding the description of all of this object's attributes
     * \return Returns all the descriptions as a Values
     */
    Values getAttributesDescriptions();

    /**
     * \brief Get the attribute synchronization method
     * \param name Attribute name
     * \return Return the synchronization method
     */
    AttributeFunctor::Sync getAttributeSyncMethod(const std::string& name);

    /**
     * Register a callback to any call to the setter
     * \param attr Attribute to add a callback to
     * \param cb Callback function
     * \return Return a callback handle
     */
    CallbackHandle registerCallback(const std::string& attr, AttributeFunctor::Callback cb);

    /**
     * Unregister a callback
     * \param handle A handle to the callback to remove
     * \return True if the callback has been successfully removed
     */
    bool unregisterCallback(const CallbackHandle& handle);

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
     * Set BaseObject::getRenderingPriority() for precision about priority
     * \param int Desired priority
     * \return Return true if the priority was set
     */
    bool setRenderingPriority(Priority priority);

    /**
     * \brief Virtual method to render the object
     */
    virtual void render() {}

    /**
     * Run the tasks waiting in the object's queue
     */
    virtual void runTasks();

  public:
    bool _savable{true}; //!< True if the object should be saved

  protected:
    unsigned long _id{0};                //!< Internal ID of the object
    std::string _type{"baseobject"};     //!< Internal type
    Category _category{Category::MISC};  //!< Object category, updated by the factory
    std::string _remoteType{""};         //!< When the object root is a Scene, this is the type of the corresponding object in the World
    std::string _name{""};               //!< Object name
    std::vector<BaseObject*> _parents{}; //!< Objects parents

    Priority _renderingPriority{Priority::NO_RENDER}; //!< Rendering priority, if negative the object won't be rendered
    int _priorityShift{0};                            //!< Shift applied to rendering priority

    bool _isConnectedToRemote{false}; //!< True if the object gets data from a World object

    RootObject* _root;                                     //!< Root object, Scene or World
    std::vector<std::weak_ptr<BaseObject>> _linkedObjects; //!< Children of this object

    std::unordered_map<std::string, AttributeFunctor> _attribFunctions; //!< Map of all attributes
    bool _updatedParams{true};                                          //!< True if the parameters have been updated and the object needs to reflect these changes

    std::future<void> _asyncTask{};
    std::mutex _asyncTaskMutex{};

    std::list<std::function<void()>> _taskQueue;
    std::recursive_mutex _taskMutex;

    bool _ghost{false}; //!< True if the object ghosts an object in another scene

    /**
     * Add a new task to the queue
     * \param task Task function
     */
    void addTask(const std::function<void()>& task);

    /**
     * \brief Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    AttributeFunctor& addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types = {});

    /**
     * \brief Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param get Get function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    AttributeFunctor& addAttribute(
        const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types = {});

    /**
     * Inform that the given object is a parent
     * \param obj Parent object
     */
    void linkToParent(BaseObject* obj);

    /**
     * Remove the given object as a parent
     * \param obj Parent object
     */
    void unlinkFromParent(BaseObject* obj);

    /**
     * Run a task asynchronously, one task at a time
     * \param func Function to run
     */
    void runAsyncTask(const std::function<void(void)>& func);

    /**
     * \brief Register new attributes
     */
    void registerAttributes();

    /**
     * \brief Set and the description for the given attribute, if it exists
     * \param name Attribute name
     * \param description Attribute description
     */
    void setAttributeDescription(const std::string& name, const std::string& description);

    /**
     * \brief Set attribute synchronization method
     * \param Method Synchronization method, can be any of the AttributeFunctor::Sync values
     */
    void setAttributeSyncMethod(const std::string& name, const AttributeFunctor::Sync& method);

    /**
     * \brief Remove the specified attribute
     * \param name Attribute name
     */
    void removeAttribute(const std::string& name);

    /**
     * \brief Set additional parameters for a given attribute
     * \param name Attribute name
     * \param savable Savability
     * \param updateDistant If true and the object has a World as root, updates the attribute of the corresponding Scene object
     */
    void setAttributeParameter(const std::string& name, bool savable, bool updateDistant);
};

} // end of namespace

#endif // SPLASH_BASE_OBJECT_H
