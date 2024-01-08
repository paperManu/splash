#include "./core/buffer_object.h"

#include "./core/root_object.h"

namespace Splash
{

/**************/
void BufferObject::setNotUpdated()
{
    GraphObject::setNotUpdated();
    _updatedBuffer = false;
}

/*************/
bool BufferObject::deserialize()
{
    if (!_newSerializedObject)
        return false;

    bool returnValue = deserialize(std::move(_serializedObject));
    _newSerializedObject = false;

    return returnValue;
}

/*************/
bool BufferObject::setSerializedObject(SerializedObject&& obj)
{
    if (obj.size() == 0)
        return false;

    // If a buffer is already being deserialized, this test will fail
    // and the value will still be true. Otherwise it will succeed, and
    // the value will be set to true.
    if (!_serializedObjectWaiting.exchange(true))
    {
        _serializedObject = std::move(obj);
        _newSerializedObject = true;

        // Deserialize it right away, in a separate thread
        _deserializeFuture = std::async(std::launch::async, [this]() {
            std::lock_guard<Spinlock> updateLock(_updateMutex);
            deserialize();
            _serializedObjectWaiting = false;
        });

        return true;
    }

    return false;
}

/*************/
void BufferObject::updateTimestamp(int64_t timestamp)
{
    std::lock_guard<Spinlock> lock(_timestampMutex);
    if (timestamp != -1)
        _timestamp = timestamp;
    else
        _timestamp = Timer::getTime();
    _updatedBuffer = true;
    if (_root)
        _root->signalBufferObjectUpdated();
}

/*************/
void BufferObject::registerAttributes()
{
    GraphObject::registerAttributes();
}

} // namespace Splash
