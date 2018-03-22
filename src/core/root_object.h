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
#include <unordered_map>

#include "./core/base_object.h"
#include "./core/factory.h"
#include "./core/link.h"

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
    /**
     * \brief Constructor
     */
    RootObject();

    /**
     * \brief Destructor
     */
    virtual ~RootObject() override;

    /**
     * Create and return an object given its type and name
     * \param type Object type
     * \param name Object name
     * \return Return a shared_ptr to the object
     */
    std::shared_ptr<BaseObject> createObject(const std::string& type, const std::string& name);

    /**
     * Delete the given object based given its name, and whether it is the last shared_ptr managing it
     * \param name Object name
     */
    void disposeObject(const std::string& name);

    /**
     * Get the object of the given name
     * \param name Object name
     * \return Return the object name
     */
    std::shared_ptr<BaseObject> getObject(const std::string& name);

    /**
     * Get the socket prefix
     * \return Return the socket prefx
     */
    std::string getSocketPrefix() const { return _linkSocketPrefix; }

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

    std::unique_ptr<Factory> _factory; //!< Object factory
    std::shared_ptr<Link> _link;       //!< Link object for communicatin between World and Scene
    std::string _linkSocketPrefix{""}; //!< Prefix to add to shared memory socket paths

    Values _lastAnswerReceived{}; //!< Holds the last answer received through the link
    std::condition_variable _answerCondition{};
    std::mutex _conditionMutex{};
    std::mutex _answerMutex{};
    std::string _answerExpected{""};

    // Condition variable for signaling a BufferObject update
    std::condition_variable _bufferObjectUpdatedCondition{};
    std::mutex _bufferObjectUpdatedMutex{};
    Spinlock _bufferObjectSingleMutex{};
    bool _bufferObjectUpdated = ATOMIC_FLAG_INIT;

    // Tasks queue
    std::mutex _recurringTaskMutex{};
    std::map<std::string, std::function<void()>> _recurringTasks{};

    mutable std::recursive_mutex _objectsMutex{};                            //!< Used in registration and unregistration of objects
    std::atomic_bool _objectsCurrentlyUpdated{false};                        //!< Prevents modification of objects from multiple places at the same time
    std::unordered_map<std::string, std::shared_ptr<BaseObject>> _objects{}; //!< Map of all the objects

    /**
     * \brief Wait for a BufferObject update. This does not prevent spurious wakeups.
     * \param timeout Timeout in us. If 0, wait indefinitely.
     * \return Return false is the timeout has been reached, true otherwise
     */
    bool waitSignalBufferObjectUpdated(uint64_t timeout = 0ull);

    /**
     * \brief Method to process a serialized object
     * \param name Object name to receive the serialized object
     * \param obj Serialized object
     */
    virtual void handleSerializedObject(const std::string& /*name*/, std::shared_ptr<SerializedObject> /*obj*/) {}

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
     * Execute all the tasks in the queue
     * Root objects can have recursive tasks, so they get their own version of this method
     */
    void runTasks() final;

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

#endif // SPLASH_ROOT_OBJECT_H
