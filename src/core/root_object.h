/*
 * Copyright (C) 2017 Splash authors
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
 * @root_object.h
 * Base class for root objects: World and Scene
 */

#ifndef SPLASH_ROOT_OBJECT_H
#define SPLASH_ROOT_OBJECT_H

#include <atomic>
#include <condition_variable>
#include <json/json.h>
#include <list>
#include <map>
#include <string>
#include <unordered_map>

#include "./config.h"
#include "./core/base_object.h"
#include "./core/factory.h"
#include "./core/graph_object.h"
#include "./core/name_registry.h"
#include "./core/tree.h"
#include "./network/link.h"
#include "./utils/dense_map.h"

namespace Splash
{

class ControllerObject;
class Queue;
class UserInput;

/*************/
class RootObject : public BaseObject
{
    // UserInput and ControllerObject can access protected members, typically _objects
    friend ControllerObject;
    friend Queue;
    friend UserInput;

  public:
    struct Context
    {
        bool defaultConfigurationFile{true};
        bool hide{false};
        bool info{false};
        bool log2file{false};
        bool childProcess{false};
        bool spawnSubprocesses{true};
        bool unitTest{false};
        std::string executableName{""};
        std::string executablePath{""};
        std::string socketPrefix{""};
        std::string childSceneName{"scene"};
        std::string configurationFile = std::string(DATADIR).append("splash.json");
        std::optional<std::string> pythonScriptPath{};
        Values pythonArgs{};
#if HAVE_LINUX
        std::optional<std::string> forcedDisplay{};
        std::string displayServer{"0"};
#endif
#if HAVE_SHMDATA
        Link::ChannelType channelType{Link::ChannelType::shmdata};
#else
        Link::ChannelType channelType{Link::ChannelType::zmq};
#endif
        std::optional<gfx::Renderer::Api> renderingApi{};
    };

    enum Command
    {
        callObject,
        callRoot
    };

  public:
    /**
     * Constructor
     */
    RootObject();

    /**
     * Constructor
     * \param context Context for the creation of this RootObject
     */
    explicit RootObject(Context context);

    /**
     * Destructor
     */
    virtual ~RootObject() override = default;

    /**
     * Other constructors/operators
     */
    RootObject(const RootObject&) = delete;
    RootObject& operator=(const RootObject&) = delete;
    RootObject(RootObject&&) = delete;
    RootObject& operator=(RootObject&&) = delete;

    /**
     * Add a command into the tree
     * \param root Target root object
     * \param cmd Command type
     * \param args Command arguments
     * \return Return true if the command has been added successfully
     */
    bool addTreeCommand(const std::string& root, Command cmd, const Values& args);

    /**
     * Create and return an object given its type and name
     * \param type Object type
     * \param name Object name
     * \return Return a shared_ptr to the object
     */
    std::weak_ptr<GraphObject> createObject(const std::string& type, const std::string& name);

    /**
     * Delete the given object based given its name, and whether it is the last shared_ptr managing it
     * \param name Object name
     */
    void disposeObject(const std::string& name);

    /**
     * Execute the commands stored in the tree
     */
    void executeTreeCommands();

    /**
     * Get the object of the given name
     * \param name Object name
     * \return Return the object name
     */
    std::shared_ptr<GraphObject> getObject(const std::string& name);

    /**
     * Get the socket prefix
     * \return Return the socket prefx
     */
    std::string getSocketPrefix() const
    {
        return _context.socketPrefix;
    }

    /**
     * Get the configuration path
     * \return Return the configuration path
     */
    std::string getConfigurationPath() const
    {
        Value value;
        _tree.getValueForLeafAt("/world/attributes/configurationPath", value);
        if (value.size() > 0 && value.getType() == Value::Type::values)
            return value[0].as<std::string>();
        return "";
    }

    /**
     * Get the media path
     * \return Return the media path
     */
    std::string getMediaPath() const
    {
        Value value;
        _tree.getValueForLeafAt("/world/attributes/mediaPath", value);
        if (value.size() > 0 && value.getType() == Value::Type::values)
            return value[0].as<std::string>();
        return "";
    }

    /**
     * Get a reference to the root tree
     * \return Return the Tree::Root
     */
    Tree::RootHandle getTree()
    {
        return _tree.getHandle();
    }

    /**
     * Set the attribute of the named object with the given args
     * \param name Object name
     * \param attrib Attribute name
     * \param args Value to set the attribute to
     * \param async Set to true for the attribute to be set asynchronously
     * \return Return true if all went well
     */
    bool set(const std::string& name, const std::string& attrib, const Values& args, bool async = true);

    /**
     * Set an object from its serialized form. If non existant, it is handled
     * by the handleSerializedObject method.
     * Note that if the object exists, this method calls itself
     * BufferObject::setFromSerializedObject, and that the deserialization is
     * handled asynchronously. Use BufferObject::hasSerializedObjectWaiting to
     * check whether a deserialization is waiting.
     * \param name Object name
     * \param obj Serialized object
     * \return Return true if the object has been set
     */
    bool setFromSerializedObject(const std::string& name, SerializedObject&& obj);

    /**
     * Send the given serialized buffer through the link
     * \param buffer Serialized buffer
     */
    void sendBuffer(SerializedObject&& buffer);

    /**
     * Return a lock object list modifications (addition, deletion)
     * \return Return a lock object which unlocks the mutex upon deletion
     */
    std::unique_lock<std::recursive_mutex> getLockOnObjects()
    {
        return std::unique_lock<std::recursive_mutex>(_objectsMutex);
    }

    /**
     * Signals that a BufferObject has been updated
     */
    void signalBufferObjectUpdated();

  protected:
    Context _context{};

    Tree::Root _tree{}; //!< Configuration / status tree, shared between all root objects
    std::unordered_map<std::string, int> _treeCallbackIds{};
    std::unordered_map<std::string, CallbackHandle> _attributeCallbackHandles{};

    std::unique_ptr<Factory> _factory{}; //!< Object factory

    Values _lastAnswerReceived{}; //!< Holds the last answer received through the link
    std::condition_variable _answerCondition{};
    std::mutex _conditionMutex{};
    std::mutex _answerMutex{};
    std::string _answerExpected{""};

    // Condition variable for signaling a BufferObject update
    std::condition_variable _bufferObjectUpdatedCondition{};
    std::mutex _bufferObjectUpdatedMutex{};
    bool _bufferObjectUpdated = ATOMIC_FLAG_INIT;

    mutable std::recursive_mutex _objectsMutex{};                   //!< Used in registration and unregistration of objects
    std::atomic_bool _objectsCurrentlyUpdated{false};               //!< Prevents modification of objects from multiple places at the same time
    DenseMap<std::string, std::shared_ptr<GraphObject>> _objects{}; //!< Map of all the objects

    std::unique_ptr<Link> _link{}; //!< Link object for communicatin between World and Scene

    /**
     * Wait for a BufferObject update. This does not prevent spurious wakeups.
     * \param timeout Timeout in us. If 0, wait indefinitely.
     * \return Return false is the timeout has been reached, true otherwise
     */
    bool waitSignalBufferObjectUpdated(uint64_t timeout = 0ull);

    /**
     * Method to process a serialized object
     *
     * Note that depending on whether the object can be handled or not, it will be
     * made invalid after calling this method. Check the return value to get whether
     * the object is still valid.
     *
     * \param name Object name to receive the serialized object
     * \param obj Serialized object
     * \return Return true if the object has been handled
     */
    virtual bool handleSerializedObject(const std::string& name, SerializedObject& obj);

    /**
     * Force the propagation of a specific path
     * \param path Path to propagate
     */
    void propagatePath(const std::string& path);

    /**
     * Propagate the Tree to peers
     */
    void propagateTree();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

    /**
     * Update the tree from the root Objects
     */
    void updateTreeFromObjects();

    /**
     * Initialize the tree
     */
    void initializeTree();

    /**
     * Converts a Value as a Json object
     * \param values Value to convert
     * \param asObject If true, return a Json object
     * \return Returns a Json object
     */
    Json::Value getValuesAsJson(const Values& values, bool asObject = false) const;

    /**
     * Save the given objects tree in a Json::Value
     * \param object Object name
     * \param rootObject Root name
     * \return Return the configuration as a Json::Value
     */
    Json::Value getObjectConfigurationAsJson(const std::string& object, const std::string& rootObject = "");

    /**
     * Save the given root object tree in a Json::Value
     * \param rootName Root name
     * \return Return the configuration as a Json::Value
     */
    Json::Value getRootConfigurationAsJson(const std::string& rootName);

    /**
     * Send a message to another root object
     * \param name Root object name
     * \param attribute Attribute name
     * \param message Message
     */
    void sendMessage(const std::string& name, const std::string& attribute, const Values& message = {})
    {
        _link->sendMessage(name, attribute, message);
    }

    /**
     * Send a message to another root object, and wait for an answer. Can specify a timeout for the answer, in microseconds.
     * \param name Root object name
     * \param attribute Attribute name
     * \param message Message
     * \param timeout Timeout in microseconds
     * \return Return the answer received (or an empty Values)
     */
    Values sendMessageWithAnswer(const std::string& name, const std::string& attribute, const Values& message = {}, const unsigned long long timeout = 0ull);
};

} // namespace Splash

#endif // SPLASH_ROOT_OBJECT_H
