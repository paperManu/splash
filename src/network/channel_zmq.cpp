#include "./network/channel_zmq.h"

#include <chrono>
#include <thread>

#include "./core/root_object.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
ChannelOutput_ZMQ::ChannelOutput_ZMQ(const RootObject* root, const std::string& name)
    : ChannelOutput(root, name)
{
    try
    {
        _socketMessageOut = std::make_unique<zmq::socket_t>(_context, ZMQ_PUB);
        _socketBufferOut = std::make_unique<zmq::socket_t>(_context, ZMQ_PUB);

        const int highWaterMark = 0;
        _socketMessageOut->set(zmq::sockopt::sndhwm, highWaterMark);
        _socketBufferOut->set(zmq::sockopt::sndhwm, highWaterMark);
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::ERROR << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
            return;
        }
    }

    _ready = true;

    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
    _pathPrefix = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");
}

/*************/
ChannelOutput_ZMQ::~ChannelOutput_ZMQ()
{
    if (!_ready)
        return;

    int lingerValue = 0;
    try
    {
        _socketMessageOut->set(zmq::sockopt::linger, lingerValue);
        _socketBufferOut->set(zmq::sockopt::linger, lingerValue);
    }
    catch (zmq::error_t& error)
    {
        Log::get() << Log::ERROR << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Error while closing socket: " << error.what() << Log::endl;
    }
}

/*************/
bool ChannelOutput_ZMQ::connectTo(const std::string& target)
{
    if (!_ready)
        return false;

    if (std::find(_targets.begin(), _targets.end(), target) != _targets.end())
        return false;

    _targets.push_back(target);

    try
    {
        _socketMessageOut->connect((_pathPrefix + "msg_" + target));
        _socketBufferOut->connect((_pathPrefix + "buf_" + target));
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::WARNING << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
            return false;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
}

/*************/
bool ChannelOutput_ZMQ::disconnectFrom(const std::string& target)
{
    if (!_ready)
        return false;

    if (std::find(_targets.begin(), _targets.end(), target) == _targets.end())
        return false;

    try
    {
        _socketMessageOut->disconnect((_pathPrefix + "msg_" + target));
        _socketBufferOut->disconnect((_pathPrefix + "buf_" + target));
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::WARNING << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Exception while disconnecting from " << target << ": " << error.what() << Log::endl;
            return false;
        }
    }

    return true;
}

/*************/
bool ChannelOutput_ZMQ::sendMessage(const std::vector<uint8_t>& message)
{
    if (!_ready)
        return false;

    try
    {
        std::lock_guard<Spinlock> lock(_msgSendMutex);
        zmq::message_t msg(message.data(), message.size());
        _socketMessageOut->send(msg, zmq::send_flags::none);
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::WARNING << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
            return false;
        }
    }

    return true;
}

/*************/
bool ChannelOutput_ZMQ::sendBuffer(SerializedObject&& buffer)
{
    if (!_ready)
        return false;

    try
    {
        std::lock_guard<Spinlock> lock(_bufferSendMutex);

        _sendQueueMutex.lock();
        _bufferSendQueue.push_back(std::move(buffer));
        auto& queuedBuffer = _bufferSendQueue.back();
        _sendQueueMutex.unlock();

        _sendQueueBufferCount++;

        zmq::message_t msg(queuedBuffer.data(), queuedBuffer.size(), ChannelOutput_ZMQ::freeSerializedBuffer, this);
        _socketBufferOut->send(msg, zmq::send_flags::none);
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::WARNING << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
            return false;
        }
    }

    return true;
}

/*************/
void ChannelOutput_ZMQ::freeSerializedBuffer(void* data, void* hint)
{
    ChannelOutput_ZMQ* ctx = reinterpret_cast<ChannelOutput_ZMQ*>(hint);

    std::unique_lock<std::mutex> lock(ctx->_bufferTransmittedMutex);

    uint32_t index = 0;
    for (; index < ctx->_bufferSendQueue.size(); ++index)
        if (ctx->_bufferSendQueue[index].data() == data)
            break;

    if (index >= ctx->_bufferSendQueue.size())
    {
        Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Buffer to free not found in currently sent buffers list" << Log::endl;
        return;
    }
    ctx->_bufferSendQueue.erase(ctx->_bufferSendQueue.begin() + index);
    ctx->_sendQueueBufferCount--;

    ctx->_bufferTransmittedCondition.notify_all();
}

/*************/
bool ChannelOutput_ZMQ::waitForBufferSending(std::chrono::milliseconds maximumWait)
{
    std::unique_lock<std::mutex> lock(_bufferTransmittedMutex);

    // If no buffer is currently being sent, return now
    if (_sendQueueBufferCount == 0)
        return true;

    const auto startTime = Timer::getTime() / 1000; // Time in ms
    auto waitDurationLeft = maximumWait.count();
    while (true)
    {
        const auto status = _bufferTransmittedCondition.wait_for(lock, std::chrono::milliseconds(waitDurationLeft));
        if (status == std::cv_status::timeout)
            return false;
        else if (_sendQueueBufferCount == 0)
            return true;
        waitDurationLeft = Timer::getTime() / 1000 - startTime;
    }

    return false;
}

/*************/
ChannelInput_ZMQ::ChannelInput_ZMQ(const RootObject* root, const std::string& name, const MessageRecvCallback& msgRecvCb, const BufferRecvCallback& bufferRecvCb)
    : ChannelInput(root, name, msgRecvCb, bufferRecvCb)
{
    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
    _pathPrefix = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");

    try
    {
        _socketMessageIn = std::make_unique<zmq::socket_t>(_context, ZMQ_SUB);
        // We don't want to miss a message: set the high water mark to a high value
        _socketMessageIn->set(zmq::sockopt::rcvhwm, 1000);
        // We subscribe to all incoming messages
        _socketMessageIn->set(zmq::sockopt::subscribe, "");
        _socketMessageIn->bind((_pathPrefix + "msg_" + _name).c_str());

        _socketBufferIn = std::make_unique<zmq::socket_t>(_context, ZMQ_SUB);
        // We only keep one buffer in memory while processing
        _socketBufferIn->set(zmq::sockopt::rcvhwm, 1);
        // We subscribe to all incoming messages
        _socketBufferIn->set(zmq::sockopt::subscribe, "");
        _socketBufferIn->bind(_pathPrefix + "buf_" + _name);
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::ERROR << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
            return;
        }
    }

    _ready = true;

    _continueListening = true;
    _bufferInThread = std::thread([&]() { handleInputBuffers(); });
    _messageInThread = std::thread([&]() { handleInputMessages(); });
}

/*************/
ChannelInput_ZMQ::~ChannelInput_ZMQ()
{
    _continueListening = false;

    if (_messageInThread.joinable())
        _messageInThread.join();

    if (_bufferInThread.joinable())
        _bufferInThread.join();
}

/*************/
void ChannelInput_ZMQ::handleInputMessages()
{
    if (!_ready)
        return;

    try
    {
        while (_continueListening)
        {
            zmq::message_t msg;
            if (!_socketMessageIn->recv(msg,
                    zmq::recv_flags::none)) // name of the target
                continue;

            std::vector<uint8_t> message((size_t)msg.size());
            std::copy(static_cast<uint8_t*>(msg.data()), static_cast<uint8_t*>(msg.data()) + msg.size(), message.data());
            _msgRecvCb(message);
        }
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
    }
}

/*************/
void ChannelInput_ZMQ::handleInputBuffers()
{
    if (!_ready)
        return;

    try
    {
        while (_continueListening)
        {
            zmq::message_t msg;
            if (!_socketBufferIn->recv(msg, zmq::recv_flags::none))
                continue;

            const auto data = static_cast<uint8_t*>(msg.data());
            auto buffer = SerializedObject(data, data + msg.size());
            _bufferRecvCb(std::move(buffer));
        }
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
    }
}

} // namespace Splash
