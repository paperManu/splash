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
#include "./core/serialized_object.h"
#include "./core/spinlock.h"

namespace Splash
{

/*************/
class BufferObject : public GraphObject
{
    /*************/
    class BufferObjectLockRead
    {
        friend BufferObject;

      public:
        ~BufferObjectLockRead()
        {
            if (_bufferObject == nullptr)
                return;
            _bufferObject->_readMutex.unlock_shared();
        }

      private:
        const BufferObject* _bufferObject;

        BufferObjectLockRead(const BufferObject* bufferObject)
            : _bufferObject(bufferObject)
        {
            if (_bufferObject == nullptr)
                return;
            _bufferObject->_readMutex.lock_shared();
        }
    };

    friend BufferObjectLockRead;

  public:
    /**
     * Constructor
     * \param root Root object
     */
    BufferObject(RootObject* root)
        : GraphObject(root)
    {
        registerAttributes();
    }

    /**
     * Get a shared read lock over this object,
     * which unlocks it upon destruction
     * \return Returns a shared read lock
     */
    BufferObjectLockRead getReadLock() { return BufferObjectLockRead(this); }

    /**
     * Set the object as dirty to force update
     */
    void setDirty() { updateTimestamp(); }

    /**
     * Check whether the object has been updated
     * \return Return true if the object has been updated
     */
    bool wasUpdated() const override
    {
        const bool updatedBuffer = _updatedBuffer;
        return updatedBuffer | GraphObject::wasUpdated();
    }

    /**
     * Set the updated buffer flag to false.
     */
    void setNotUpdated() override;

    /**
     * Update the BufferObject from a serialized representation.
     * \param obj Serialized object to use as source
     * \return Return true if everything went well
     */
    virtual bool deserialize(SerializedObject&& /*obj*/) = 0;

    /**
     * Update the BufferObject from the inner serialized object, set with setSerializedObject
     * \return Return true if everything went well
     */
    bool deserialize();

    /**
     * Get the name of the distant buffer object, for those which have a different name between World and Scene (happens with Queues)
     * \return Return the distant name
     */
    virtual std::string getDistantName() const { return _name; }

    /**
     * Get the timestamp for the current buffer object
     * \return Return the timestamp
     */
    virtual int64_t getTimestamp() const override
    {
        std::lock_guard<Spinlock> lock(_timestampMutex);
        return _timestamp;
    }

    /**
     * Set the timestamp
     * \param timestamp Timestamp, in us
     */
    virtual void setTimestamp(int64_t timestamp) override
    {
        std::lock_guard<Spinlock> lock(_timestampMutex);
        _timestamp = timestamp;
    }

    /**
     * Serialize the object
     * \return Return a serialized representation of the object
     */
    virtual SerializedObject serialize() const = 0;

    /**
     * Set the next serialized object to deserialize to buffer. Deserialization is
     * done asynchronously, in a separate thread if the system allows for it. Use
     * hasSerializedObjectWaiting to check whether a deserialization is waiting
     * If another object is currently being set for deserialization, the call
     * to this method will do nothing and return false.
     * \param obj Serialized object
     * \return Return true if the object has been set for deserialization, false otherwise
     */
    bool setSerializedObject(SerializedObject&& obj);

    /**
     * Check whether a serialized object is waiting for deserialization
     * \return Return true if a serialized object is waiting
     */
    bool hasSerializedObjectWaiting() const { return _newSerializedObject; };

  protected:
    /**
     * Update mutex, used internally to prevent
     * the buffer to be updated from multiple places
     * at the same time
     */
    mutable Spinlock _updateMutex; //!< Update mutex, which prevents content buffer switcher

    /**
     * Read mutex, used to prevent the buffer content to
     * be modified while it is being read
     */
    mutable std::shared_mutex _readMutex;

    std::atomic_bool _serializedObjectWaiting{false}; //!< True if a serialized object has been set and waits for processing
    std::future<void> _deserializeFuture{};           //!< Holds the deserialization thread
    mutable Spinlock _timestampMutex;
    int64_t _timestamp{0};                  //!< Timestamp
    std::atomic_bool _updatedBuffer{false}; //!< True if the BufferObject has been updated

    SerializedObject _serializedObject;           //!< Internal buffer object
    std::atomic_bool _newSerializedObject{false}; //!< Set to true during serialized object processing

    /**
     * Updates the timestamp of the object. Also, set the update flag to true.
     * \param timestamp Value to set the timestamp to, -1 to set to the current time
     */
    virtual void updateTimestamp(int64_t timestamp = -1);

    /**
     * Register new attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_BUFFER_OBJECT_H
