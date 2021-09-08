#include "./network/link.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./core/constants.h"
#include "./core/root_object.h"
#include "./network/channel_zmq.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Link::Link(RootObject* root, const std::string& name)
    : _rootObject(root)
    , _name(name)
    , _channelOutput(std::make_unique<ChannelOutput_ZMQ>(root, name))
    , _channelInput(std::make_unique<ChannelInput_ZMQ>(
          root,
          name,
          [&](const std::string& name, const std::string& attribute, const Values& value) { handleInputMessages(name, attribute, value); },
          [&](const std::string& name, SerializedObject&& buffer) { handleInputBuffers(name, std::move(buffer)); }))
{
}

/*************/
Link::~Link()
{
}

/*************/
void Link::connectTo(const std::string& name)
{
    _channelOutput->connectTo(name);
}

/*************/
void Link::disconnectFrom(const std::string& name)
{
    _channelOutput->disconnectFrom(name);
}

/*************/
bool Link::waitForBufferSending(std::chrono::milliseconds maximumWait)
{
    return _channelOutput->waitForBufferSending(maximumWait);
}

/*************/
bool Link::sendBuffer(const std::string& name, SerializedObject&& buffer)
{
    return _channelOutput->sendBufferTo(name, std::move(buffer));
}

/*************/
bool Link::sendBuffer(const std::string& name, const std::shared_ptr<BufferObject>& object)
{
    auto buffer = object->serialize();
    return sendBuffer(name, std::move(buffer));
}

/*************/
bool Link::sendMessage(const std::string& name, const std::string& attribute, const Values& message)
{
    auto result = _channelOutput->sendMessageTo(name, attribute, message);

#ifdef DEBUG
    // We don't display broadcast messages, for visibility
    if (name != Constants::ALL_PEERS)
        Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Sending message to " << name << "::" << attribute << Log::endl;
#endif

    return result;
}

/*************/
void Link::handleInputMessages(const std::string& name, const std::string& attribute, const Values& value)
{
    if (_rootObject)
        _rootObject->set(name, attribute, value);
#ifdef DEBUG
    // We don't display broadcast messages, for visibility
    if (name != Constants::ALL_PEERS)
        Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " (" << _rootObject->getName() << ")"
                   << " - Receiving message for " << name << "::" << attribute << Log::endl;
#endif
}

/*************/
void Link::handleInputBuffers(const std::string& name, SerializedObject&& buffer)
{
    if (_rootObject)
        _rootObject->setFromSerializedObject(name, std::move(buffer));
}

} // namespace Splash
