#include "./buffer_object.h"

#include "./root_object.h"

using namespace std;

namespace Splash
{

/**************/
void BufferObject::setNotUpdated()
{
    BaseObject::setNotUpdated();
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
    if (_serializedObjectWaiting.compare_exchange_strong(expectedAtomicValue, true))
    {
        _serializedObject = move(obj);
        _newSerializedObject = true;

        // Deserialize it right away, in a separate thread
        SThread::pool.enqueueWithoutId([this]() {
            lock_guard<Spinlock> lock(_writeMutex);
            deserialize();
            _serializedObjectWaiting = false;
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
