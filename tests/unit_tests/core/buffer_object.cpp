#include <chrono>
#include <thread>

#include <doctest.h>

#include "./core/buffer_object.h"

using namespace Splash;

/*************/
class BufferObjectMock : public BufferObject
{
  public:
    BufferObjectMock()
        : BufferObject(nullptr)
    {
    }

    bool tryLockWrite()
    {
        if (_writeMutex.try_lock())
            return true;
        return false;
    }

    bool deserialize(const std::shared_ptr<SerializedObject>& obj) final
    {
        updateTimestamp();
        return true;
    }

    std::shared_ptr<SerializedObject> serialize() const final { return nullptr; }
};

/*************/
TEST_CASE("Testing BufferObject locking")
{
    auto buffer = BufferObjectMock();
    buffer.lockWrite();
    CHECK(!buffer.tryLockWrite());
    buffer.unlockWrite();
    CHECK(buffer.tryLockWrite());
}

/*************/
TEST_CASE("Testing timestamp and update flag")
{
    auto buffer = BufferObjectMock();
    auto timestamp = buffer.getTimestamp();

    buffer.setDirty();
    CHECK_NE(timestamp, buffer.getTimestamp());
    CHECK(buffer.wasUpdated());

    buffer.setDirty();
    buffer.setNotUpdated();
    CHECK(!buffer.wasUpdated());

    buffer.setTimestamp(timestamp);
    CHECK_EQ(timestamp, buffer.getTimestamp());
}

/*************/
TEST_CASE("Testing serialization")
{
    auto buffer = BufferObjectMock();
    auto timestamp = buffer.getTimestamp();

    buffer.setSerializedObject({nullptr});
    while (buffer.hasSerializedObjectWaiting())
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    CHECK_NE(timestamp, buffer.getTimestamp());
}
