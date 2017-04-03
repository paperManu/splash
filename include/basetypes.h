/*
 * Copyright (C) 2015 Emmanuel Durand
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
 * @basetypes.h
 * Base types, from which more complex classes derive
 */

#ifndef SPLASH_BASETYPES_H
#define SPLASH_BASETYPES_H

#include <atomic>
#include <condition_variable>
#include <json/json.h>
#include <list>
#include <map>
#include <unordered_map>

#include "./coretypes.h"
#include "./link.h"
#include "./log.h"
#include "./timer.h"

namespace Splash
{

/*************/
//! AttributeFunctor class, used to add attributes to class through setter and getter functions.
struct AttributeFunctor
{
  public:
    enum class Sync
    {
        no_sync,
        force_sync,
        force_async
    };

    /**
     * \brief Default constructor.
     */
    AttributeFunctor(){};

    /**
     * \brief Constructor.
     * \param name Name of the attribute.
     * \param setFunc Setter function.
     * \param types Vector of char defining the parameters types the setter function expects.
     */
    AttributeFunctor(const std::string& name, const std::function<bool(const Values&)>& setFunc, const std::vector<char>& types = {});

    /**
     * \brief Constructor.
     * \param name Name of the attribute.
     * \param setFunc Setter function.
     * \param getFunc Getter function.
     * \param types Vector of char defining the parameters types the setter function expects.
     */
    AttributeFunctor(const std::string& name, const std::function<bool(const Values&)>& setFunc, const std::function<const Values()>& getFunc, const std::vector<char>& types = {});

    AttributeFunctor(const AttributeFunctor&) = delete;
    AttributeFunctor& operator=(const AttributeFunctor&) = delete;
    AttributeFunctor(AttributeFunctor&& a) { operator=(std::move(a)); }
    AttributeFunctor& operator=(AttributeFunctor&& a);

    /**
     * \brief Parenthesis operator which calls the setter function if defined, otherwise calls a default setter function which only stores the arguments if the have the right type.
     * \param args Arguments as a queue of Value.
     * \return Returns true if the set did occur.
     */
    bool operator()(const Values& args);

    /**
     * \brief Parenthesis operator which calls the getter function if defined, otherwise simply returns the stored values.
     * \return Returns the stored values.
     */
    Values operator()() const;

    /**
     * \brief Tells whether the setter and getters are the default ones or not.
     * \return Returns true if the setter and getter are the default ones.
     */
    bool isDefault() const { return _defaultSetAndGet; }

    /**
     * \brief Ask whether to update the Scene object (if this attribute is hosted by a World object).
     * \return Returns true if the World should update this attribute in the distant Scene object.
     */
    bool doUpdateDistant() const { return _doUpdateDistant; }

    /**
     * \brief Set whether to update the Scene object (if this attribute is hosted by a World object).
     * \return Returns true if the World should update this attribute in the distant Scene object.
     */
    void doUpdateDistant(bool update) { _doUpdateDistant = update; }

    /**
     * \brief Get the types of the wanted arguments.
     * \return Returns the expected types in a Values.
     */
    Values getArgsTypes() const;

    /**
     * \brief Ask whether the attribute is locked.
     * \return Returns true if the attribute is locked.
     */
    bool isLocked() const { return _isLocked; }

    /**
     * \brief Lock the attribute to the given value.
     * \param v The value to set the attribute to. If empty, uses the stored value.
     * \return Returns true if the value could be locked.
     */
    bool lock(const Values& v = {});

    /**
     * \brief Unlock the attribute.
     */
    void unlock() { _isLocked = false; }

    /**
     * \brief Ask whether the attribute should be saved.
     * \return Returns true if the attribute should be saved.
     */
    bool savable() const { return _savable; }

    /**
     * \brief Set whether the attribute should be saved.
     * \param save If true, the attribute will be save.
     */
    void savable(bool save) { _savable = save; }

    /**
     * \brief Set the description.
     * \param desc Description.
     */
    void setDescription(const std::string& desc) { _description = desc; }

    /**
     * \brief Get the description.
     * \return Returns the description.
     */
    std::string getDescription() const { return _description; }

    /**
     * \brief Set the name of the object holding this attribute
     * \param objectName Name of the parent object
     */
    void setObjectName(const std::string& objectName) { _objectName = objectName; }

    /**
     * \brief Get the synchronization method for this attribute
     * \return Return the synchronization method
     */
    Sync getSyncMethod() const { return _syncMethod; }

    /**
     * \brief Set the prefered synchronization method for this attribute
     * \param method Can be Sync::no_sync, Sync::force_sync, Sync::force_async
     */
    void setSyncMethod(const Sync& method) { _syncMethod = method; }

  private:
    mutable std::mutex _defaultFuncMutex{};
    std::function<bool(const Values&)> _setFunc{};
    std::function<const Values()> _getFunc{};

    std::string _objectName;         // Name of the object holding this attribute
    std::string _name;               // Name of the attribute
    std::string _description{};      // Attribute description
    Values _values;                  // Holds the values for the default set and get functions
    std::vector<char> _valuesTypes;  // List of the types held in _values
    Sync _syncMethod{Sync::no_sync}; //!< Synchronization to consider while setting this attribute

    bool _isLocked{false};

    bool _defaultSetAndGet{true};
    bool _doUpdateDistant{false}; // True if the World should send this attr values to Scenes
    bool _savable{true};          // True if this attribute should be saved
};

class BaseObject;
class RootObject;

/*************/
//! BaseObject class, which is the base class for all of classes
class BaseObject
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
        MESH
    };

  public:
    /**
     * \brief Constructor.
     */
    BaseObject() { init(); }

    /**
     * \brief Constructor.
     * \param root Specify the root object.
     */
    BaseObject(const std::weak_ptr<RootObject>& root)
        : _root(root)
    {
        init();
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
    bool setAttribute(const std::string& attrib, const Values& args);

    /**
     * \brief Get the specified attribute
     * \param attrib Attribute name
     * \param args Values object which will hold the attribute values
     * \param includeDistant Return true even if the attribute is distant
     * \param includeNonSavable Return true even if the attribute is not savable
     * \return Return true if the parameter exists
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
     * \brief Update the content of the object
     */
    virtual void update() {}

    /**
     * \brief Converts a Value as a Json object
     * \param values Value to convert
     * \return Returns a Json object
     */
    Json::Value getValuesAsJson(const Values& values) const;

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

  public:
    bool _savable{true}; //!< True if the object should be saved

  protected:
    unsigned long _id{0};               //!< Internal ID of the object
    std::string _type{"baseobject"};    //!< Internal type
    Category _category{Category::MISC}; //!< Object category, updated by the factory
    std::string _remoteType{""};        //!< When the object root is a Scene, this is the type of the corresponding object in the World
    std::string _name{""};              //!< Object name

    Priority _renderingPriority{Priority::NO_RENDER}; //!< Rendering priority, if negative the object won't be rendered
    int _priorityShift{0};                            //!< Shift applied to rendering priority

    bool _isConnectedToRemote{false}; //!< True if the object gets data from a World object

    std::weak_ptr<RootObject> _root;                       //!< Root object, Scene or World
    std::vector<std::weak_ptr<BaseObject>> _linkedObjects; //!< Children of this object

    std::unordered_map<std::string, AttributeFunctor> _attribFunctions; //!< Map of all attributes
    bool _updatedParams{true};                                          //!< True if the parameters have been updated and the object needs to reflect these changes

    /**
     * \brief Initialize some generic attributes
     */
    void init();

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

/*************/
//! Base class for buffer objects, which are updated from outside sources. Typically, videos or live meshes
class BufferObject : public BaseObject
{
  public:
    /**
     * \brief Constructor
     */
    BufferObject() {}

    /**
     * \brief Constructor
     * \param root Root object
     */
    BufferObject(const std::weak_ptr<RootObject>& root)
        : BaseObject(root)
    {
        registerAttributes();
    }

    /**
     * \brief Destructor
     */
    virtual ~BufferObject() override {}

    /**
     * \brief Check whether the object has been updated
     * \return Return true if the object has been updated
     */
    bool wasUpdated() const { return _updatedBuffer | BaseObject::wasUpdated(); }

    /**
     * \brief Set the updated buffer flag to false.
     */
    void setNotUpdated();

    /**
     * \brief Update the BufferObject from a serialized representation.
     * \param obj Serialized object to use as source
     * \return Return true if everything went well
     */
    virtual bool deserialize(const std::shared_ptr<SerializedObject>& obj) = 0;

    /**
     * \brief Update the BufferObject from the inner serialized object, set with setSerializedObject
     * \return Return true if everything went well
     */
    bool deserialize();

    /**
     * \brief Get the name of the distant buffer object, for those which have a different name between World and Scene (happens with Queues)
     * \return Return the distant name
     */
    virtual std::string getDistantName() const { return _name; }

    /**
     * \brief Get the timestamp for the current buffer object
     * \return Return the timestamp
     */
    int64_t getTimestamp() const { return _timestamp; }

    /**
     * \brief Serialize the object
     * \return Return a serialized representation of the object
     */
    virtual std::shared_ptr<SerializedObject> serialize() const = 0;

    /**
     * \brief Set the next serialized object to deserialize to buffer
     * \param obj Serialized object
     */
    void setSerializedObject(std::shared_ptr<SerializedObject> obj);

    /**
     * \brief Updates the timestamp of the object. Also, set the update flag to true.
     */
    void updateTimestamp();

  protected:
    mutable Spinlock _readMutex;                      //!< Read mutex locked when the object is read from
    mutable Spinlock _writeMutex;                     //!< Write mutex locked when the object is written to
    std::atomic_bool _serializedObjectWaiting{false}; //!< True if a serialized object has been set and waits for processing
    int64_t _timestamp{0};                            //!< Timestamp
    bool _updatedBuffer{false};                       //!< True if the BufferObject has been updated

    std::shared_ptr<SerializedObject> _serializedObject; //!< Internal buffer object
    bool _newSerializedObject{false};                    //!< Set to true during serialized object processing

    /**
     * \brief Register new attributes
     */
    void registerAttributes() { BaseObject::registerAttributes(); }
};

class UserInput;
class ControllerObject;

/*************/
//! Base class for root objects: World and Scene
class RootObject : public BaseObject
{
    // UserInput and ControllerObject can access protected members, typically _objects
    friend UserInput;
    friend ControllerObject;

  public:
    /**
     * \brief Constructor
     */
    RootObject();

    /**
     * \brief Destructor
     */
    virtual ~RootObject() override {}

    /**
     * \brief Get the configuration path
     * \return Return the configuration path
     */
    std::string getConfigurationPath() const { return _configurationPath; }

    /**
     * \brief Get the media path
     * \return Return the media path
     */
    std::string getMediaPath() const { return _mediaPath; }

    /**
     * \brief Register an object which was created elsewhere. If an object was the same name exists, it is replaced.
     * \param object Object to register
     */
    void registerObject(std::shared_ptr<BaseObject> object);

    /**
     * \brief Unregister an object which was created elsewhere, from its name, sending back a shared_ptr for it.
     * \param name Object name
     * \return Return a shared pointer to the unregistered object
     */
    std::shared_ptr<BaseObject> unregisterObject(const std::string& name);

    /**
     * \brief Set the attribute of the named object with the given args
     * \param name Object name
     * \param attrib Attribute name
     * \param args Value to set the attribute to
     * \param async Set to true for the attribute to be set asynchronously
     * \return Return true if all went well
     */
    bool set(const std::string& name, const std::string& attrib, const Values& args, bool async = true);

    /**
     * \brief Set an object from its serialized form. If non existant, it is handled by the handleSerializedObject method.
     * \param name Object name
     * \param obj Serialized object
     */
    void setFromSerializedObject(const std::string& name, std::shared_ptr<SerializedObject> obj);

    /**
     * \brief Send the given serialized buffer through the link
     * \param name Destination BufferObject name
     * \param buffer Serialized buffer
     */
    void sendBuffer(const std::string& name, const std::shared_ptr<SerializedObject>& buffer) { _link->sendBuffer(name, buffer); }

    /**
     * \brief Return a lock object list modifications (addition, deletion)
     * \return Return a lock object which unlocks the mutex upon deletion
     */
    std::unique_lock<std::recursive_mutex> getLockOnObjects() { return std::move(std::unique_lock<std::recursive_mutex>(_objectsMutex)); }

    /**
     * \brief Signals that a BufferObject has been updated
     */
    void signalBufferObjectUpdated();

  protected:
    std::string _configurationPath{""}; //!< Path to the configuration file
    std::string _mediaPath{""};         //!< Default path to the medias

    std::shared_ptr<Link> _link;                                           //!< Link object for communicatin between World and Scene
    mutable std::recursive_mutex _objectsMutex;                            //!< Used in registration and unregistration of objects
    std::atomic_bool _objectsCurrentlyUpdated{false};                      //!< Prevents modification of objects from multiple places at the same time
    std::unordered_map<std::string, std::shared_ptr<BaseObject>> _objects; //!< Map of all the objects

    Values _lastAnswerReceived{}; //!< Holds the last answer received through the link
    std::condition_variable _answerCondition;
    std::mutex _conditionMutex;
    std::mutex _answerMutex;
    std::string _answerExpected{""};

    // Condition variable for signaling a BufferObject update
    std::condition_variable _bufferObjectUpdatedCondition;
    std::mutex _bufferObjectUpdatedMutex;

    // Tasks queue
    std::recursive_mutex _taskMutex;
    std::list<std::function<void()>> _taskQueue;
    std::mutex _recurringTaskMutex;
    std::map<std::string, std::function<void()>> _recurringTasks;

    /**
     * \brief Wait for a BufferObject update. This does not prevent spurious wakeups.
     * \param timeout Timeout in us
     * \return Return false is the timeout has been reached, true otherwise
     */
    bool waitSignalBufferObjectUpdated(uint64_t timeout);

    /**
     * \brief Method to process a serialized object
     * \param name Object name to receive the serialized object
     * \param obj Serialized object
     */
    virtual void handleSerializedObject(const std::string& name, std::shared_ptr<SerializedObject> obj) {}

    /**
     * \brief Add a new task to the queue
     * \param task Task function
     */
    void addTask(const std::function<void()>& task);

    /**
     * Add a task repeated at each frame
     * \param name Task name
     * \param task Task function
     */
    void addRecurringTask(const std::string& name, const std::function<void()>& task);

    /**
     * Remove a recurring task
     * \param name Task name
     */
    void removeRecurringTask(const std::string& name);

    /**
     * \brief Execute all the tasks in the queue
     */
    void runTasks();

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();

    /**
     * \brief Send a message to another root object
     * \param name Root object name
     * \param attribute Attribute name
     * \param message Message
     */
    void sendMessage(const std::string& name, const std::string& attribute, const Values& message = {}) { _link->sendMessage(name, attribute, message); }

    /**
     * \brief Send a message to another root object, and wait for an answer. Can specify a timeout for the answer, in microseconds.
     * \param name Root object name
     * \param attribute Attribute name
     * \param message Message
     * \param timeout Timeout in microseconds
     * \return Return the answer received (or an empty Values)
     */
    Values sendMessageWithAnswer(const std::string& name, const std::string& attribute, const Values& message = {}, const unsigned long long timeout = 0ull);
};

} // end of namespace

#endif // SPLASH_BASETYPES_H
