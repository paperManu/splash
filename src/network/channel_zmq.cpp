#include "./network/channel_zmq.h"

#include <chrono>
#include <thread>

#include "./core/root_object.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

#if HAVE_WINDOWS
zmq::context_t zmqCommonContext{0};
#endif

/*************/
ChannelOutput_ZMQ::ChannelOutput_ZMQ(const RootObject* root, const std::string& name)
    : ChannelOutput(root, name)
{
    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
#if HAVE_LINUX
    _pathPrefix = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");
#elif HAVE_WINDOWS
    _pathPrefix = "inproc://splash_";
#endif

    try
    {
#if HAVE_LINUX
        _context = zmq::context_t(1);
        _socketMessageOut = std::make_unique<zmq::socket_t>(_context, ZMQ_PUB);
        _socketBufferOut = std::make_unique<zmq::socket_t>(_context, ZMQ_PUB);
#elif HAVE_WINDOWS
        _socketMessageOut = std::make_unique<zmq::socket_t>(zmqCommonContext, ZMQ_PUB);
        _socketBufferOut = std::make_unique<zmq::socket_t>(zmqCommonContext, ZMQ_PUB);
#endif

        const int highWaterMark = 0;
        _socketMessageOut->set(zmq::sockopt::sndhwm, highWaterMark);
        _socketBufferOut->set(zmq::sockopt::sndhwm, highWaterMark);

        _socketMessageOut->bind(_pathPrefix + "msg_" + _name);
        _socketBufferOut->bind(_pathPrefix + "buf_" + _name);
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
}

/*************/
ChannelOutput_ZMQ::~ChannelOutput_ZMQ()
{
    if (!_ready)
        return;

    try
    {
        const int lingerValue = 0;
        _socketMessageOut->set(zmq::sockopt::linger, lingerValue);
        _socketBufferOut->set(zmq::sockopt::linger, lingerValue);
    }
    catch (zmq::error_t& error)
    {
        Log::get() << Log::ERROR << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Error while closing socket: " << error.what() << Log::endl;
    }
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
#if HAVE_LINUX
    _pathPrefix = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");
#elif HAVE_WINDOWS
    _pathPrefix = "inproc://splash_";
#endif

    try
    {
#if HAVE_LINUX
        _context = zmq::context_t(1);
        _socketMessageIn = std::make_unique<zmq::socket_t>(_context, ZMQ_SUB);
        _socketBufferIn = std::make_unique<zmq::socket_t>(_context, ZMQ_SUB);
#elif HAVE_WINDOWS
        _socketMessageIn = std::make_unique<zmq::socket_t>(zmqCommonContext, ZMQ_SUB);
        _socketBufferIn = std::make_unique<zmq::socket_t>(zmqCommonContext, ZMQ_SUB);
#endif
        // We don't want to miss a message: set the high water mark to a high value
        _socketMessageIn->set(zmq::sockopt::rcvhwm, 1000);
        // We only keep one buffer in memory while processing
        _socketBufferIn->set(zmq::sockopt::rcvhwm, 1);
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
        {
            Log::get() << Log::ERROR << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
            return;
        }
    }

    _continueListening = true;
    _messageInThread = std::thread([&]() { handleInputMessages(); });
    _bufferInThread = std::thread([&]() { handleInputBuffers(); });

    _ready = true;
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
[[nodiscard]] bool ChannelInput_ZMQ::connectTo(const std::string& target)
{
    if (!_ready)
        return false;

    if (_targets.find(target) != _targets.end())
        return false;

    _targets.insert(target);

    try
    {
        _socketMessageIn->connect(_pathPrefix + "msg_" + target);
        _socketBufferIn->connect(_pathPrefix + "buf_" + target);

        // We subscribe to all incoming messages
        _socketMessageIn->set(zmq::sockopt::subscribe, "");
        _socketBufferIn->set(zmq::sockopt::subscribe, "");
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
[[nodiscard]] bool ChannelInput_ZMQ::disconnectFrom(const std::string& target)
{
    if (!_ready)
        return false;

    if (_targets.erase(target) == 0)
        return false;

    try
    {
        _socketMessageIn->disconnect((_pathPrefix + "msg_" + target));
        _socketBufferIn->disconnect((_pathPrefix + "buf_" + target));
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
void ChannelInput_ZMQ::handleInputMessages()
{
    try
    {
        zmq::pollitem_t items = {_socketMessageIn->handle(), 0, ZMQ_POLLIN, 0};

        while (_continueListening)
        {
            if (!zmq::poll(&items, 1, std::chrono::milliseconds(10)))
                continue;

            zmq::message_t msg;
            while (_socketMessageIn->recv(msg, zmq::recv_flags::dontwait))
            {
                std::vector<uint8_t> message(static_cast<size_t>(msg.size()));
                std::copy(static_cast<uint8_t*>(msg.data()), static_cast<uint8_t*>(msg.data()) + msg.size(), message.data());
                _msgRecvCb(message);
            }
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
    try
    {
        zmq::pollitem_t items = {_socketBufferIn->handle(), 0, ZMQ_POLLIN, 0};

        while (_continueListening)
        {
            if (!zmq::poll(&items, 1, std::chrono::milliseconds(10)))
                continue;

            zmq::message_t msg;
            while (_socketBufferIn->recv(msg, zmq::recv_flags::dontwait))
            {
                const auto data = static_cast<uint8_t*>(msg.data());
                auto buffer = SerializedObject(data, data + msg.size());
                _bufferRecvCb(std::move(buffer));
            }
        }
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
    }
}

} // namespace Splash
