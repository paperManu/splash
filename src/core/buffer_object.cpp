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
    bool expectedAtomicValue = false;
    if (_serializedObjectWaiting.compare_exchange_strong(expectedAtomicValue, true, std::memory_order_acq_rel))
    {
        _serializedObject = move(obj);
        _newSerializedObject = true;

        // Deserialize it right away, in a separate thread
        _deserializeFuture = async(launch::async, [this]() {
            lock_guard<shared_timed_mutex> lock(_writeMutex);
            deserialize();
            _serializedObjectWaiting.store(false, std::memory_order_acq_rel);
        });
    }
}

/*************/
void BufferObject::updateTimestamp()
{
    _timestamp = Timer::getTime();
    _updatedBuffer = true;
    if (_root)
        _root->signalBufferObjectUpdated();
}

} // end of namespace
