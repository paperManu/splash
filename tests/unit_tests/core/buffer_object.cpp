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

    bool tryLockRead()
    {
        if (_readMutex.try_lock())
            return true;
        return false;
    }

    bool deserialize(SerializedObject&& obj) final
    {
        updateTimestamp();
        return true;
    }

    SerializedObject serialize() const final { return {}; }
};

/*************/
TEST_CASE("Testing BufferObject locking")
{
    auto buffer = BufferObjectMock();
    {
        const auto readLock = buffer.getReadLock();
        CHECK(!buffer.tryLockRead());
    }
    CHECK(buffer.tryLockRead());
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

    auto result = buffer.setSerializedObject({});
    CHECK_EQ(result, false);
    CHECK_EQ(timestamp, buffer.getTimestamp());
}
