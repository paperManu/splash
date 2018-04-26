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
 * @buffer.h
 * Buffer objects, able to be serialized and sent to other processes
 */

#ifndef SPLASH_BUFFER_OBJECT_H
#define SPLASH_BUFFER_OBJECT_H

#include <atomic>
#include <condition_variable>
#include <future>
#include <json/json.h>
#include <list>
#include <map>
#include <shared_mutex>
#include <unordered_map>

#include "./core/graph_object.h"
#include "./core/spinlock.h"

namespace Splash
{

/*************/
class BufferObject : public GraphObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    BufferObject(RootObject* root)
        : GraphObject(root)
    {
        registerAttributes();
    }

    /**
     * Lock the buffer, useful while reading. Use with care
     * Note that only write mutex is needed, as it also disables reading
     * Can be locked multiple times, must then be unlocked as many times
     */
    void lockWrite() { _writeMutex.lock_shared(); }

    /**
     * \brief Unlock the buffer
     */
    void unlockWrite() { _writeMutex.unlock_shared(); }

    /**
     * Set the object as dirty to force update
     */
    void setDirty() { updateTimestamp(); }

    /**
     * \brief Check whether the object has been updated
     * \return Return true if the object has been updated
     */
    bool wasUpdated() const { return _updatedBuffer | GraphObject::wasUpdated(); }

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

  protected:
    mutable Spinlock _readMutex;                      //!< Read mutex locked when the object is read from
    mutable std::shared_timed_mutex _writeMutex;      //!< Write mutex locked when the object is written to
    std::atomic_bool _serializedObjectWaiting{false}; //!< True if a serialized object has been set and waits for processing
    std::future<void> _deserializeFuture{};           //!< Holds the deserialization thread
    int64_t _timestamp{0};                            //!< Timestamp
    bool _updatedBuffer{false};                       //!< True if the BufferObject has been updated

    std::shared_ptr<SerializedObject> _serializedObject{nullptr}; //!< Internal buffer object
    bool _newSerializedObject{false};                             //!< Set to true during serialized object processing

    /**
     * \brief Updates the timestamp of the object. Also, set the update flag to true.
     */
    void updateTimestamp();

    /**
     * \brief Register new attributes
     */
    void registerAttributes() { GraphObject::registerAttributes(); }
};

} // end of namespace

#endif // SPLASH_BUFFER_OBJECT_H
