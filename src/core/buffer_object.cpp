#include "./core/buffer_object.h"

#include "./core/root_object.h"

using namespace std;

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

    bool returnValue = deserialize(_serializedObject);
    _newSerializedObject = false;

    return returnValue;
}

/*************/
void BufferObject::setSerializedObject(shared_ptr<SerializedObject> obj)
{
    if (_serializedObjectWaitingMutex.try_lock())
    {
        _serializedObject = move(obj);
        _newSerializedObject = true;

        // Deserialize it right away, in a separate thread
        _deserializeFuture = async(launch::async, [this]() {
            lock_guard<shared_mutex> lock(_writeMutex);
            deserialize();
            _serializedObjectWaitingMutex.unlock();
        });
    }
}

/*************/
void BufferObject::updateTimestamp(int64_t timestamp)
{
    lock_guard<mutex> lock(_timestampMutex);
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
