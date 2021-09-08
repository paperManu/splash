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
            Log::get() << Log::WARNING << "ChannelOutput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
    }

    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
    _pathPrefix = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");
}

/*************/
ChannelOutput_ZMQ::~ChannelOutput_ZMQ()
{
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
bool ChannelOutput_ZMQ::sendMessageTo(const std::string& name, const std::string& attribute, const Values& value)
{
    try
    {
        std::lock_guard<Spinlock> lock(_msgSendMutex);

        // First we send the name of the target
        zmq::message_t msg(name.size() + 1);
        memcpy(msg.data(), (void*)name.c_str(), name.size() + 1);
        _socketMessageOut->send(msg, zmq::send_flags::sndmore);

        // And the target's attribute
        msg.rebuild(attribute.size() + 1);
        memcpy(msg.data(), (void*)attribute.c_str(), attribute.size() + 1);
        _socketMessageOut->send(msg, zmq::send_flags::sndmore);

        // Helper function to send messages
        std::function<void(const Values& message)> sendMessage;
        sendMessage = [&](const Values& message) {
            // Size of the message
            int size = message.size();
            msg.rebuild(sizeof(size));
            memcpy(msg.data(), (void*)&size, sizeof(size));

            if (message.size() == 0)
                _socketMessageOut->send(msg, zmq::send_flags::none);
            else
                _socketMessageOut->send(msg, zmq::send_flags::sndmore);

            for (uint32_t i = 0; i < message.size(); ++i)
            {
                auto v = message[i];
                Value::Type valueType = v.getType();

                msg.rebuild(sizeof(valueType));
                memcpy(msg.data(), (void*)&valueType, sizeof(valueType));
                _socketMessageOut->send(msg, zmq::send_flags::sndmore);

                std::string valueName = v.getName();
                msg.rebuild(valueName.size() + 1);
                memcpy(msg.data(), valueName.data(), valueName.size() + 1);
                _socketMessageOut->send(msg, zmq::send_flags::sndmore);

                if (valueType == Value::Type::values)
                    sendMessage(v.as<Values>());
                else
                {
                    int valueSize = (valueType == Value::Type::string) ? v.byte_size() + 1 : v.byte_size();
                    void* value = v.data();
                    msg.rebuild(valueSize);
                    memcpy(msg.data(), value, valueSize);

                    if (i != message.size() - 1)
                        _socketMessageOut->send(msg, zmq::send_flags::sndmore);
                    else
                        _socketMessageOut->send(msg, zmq::send_flags::none);
                }
            }
        };

        // Send the message
        sendMessage(value);
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
bool ChannelOutput_ZMQ::sendBufferTo(const std::string& name, SerializedObject&& buffer)
{
    try
    {
        std::lock_guard<Spinlock> lock(_bufferSendMutex);

        _sendQueueMutex.lock();
        _bufferSendQueue.push_back(std::move(buffer));
        auto& queuedBuffer = _bufferSendQueue.back();
        _sendQueueMutex.unlock();

        _sendQueueBufferCount++;

        zmq::message_t msg(name.size() + 1);
        memcpy(msg.data(), (void*)name.c_str(), name.size() + 1);
        _socketBufferOut->send(msg, zmq::send_flags::sndmore);

        msg.rebuild(queuedBuffer.data(), queuedBuffer.size(), ChannelOutput_ZMQ::freeSerializedBuffer, this);
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
ChannelInput_ZMQ::ChannelInput_ZMQ(const RootObject* root, const std::string& name, MessageRecvCallback msgRecvCb, BufferRecvCallback bufferRecvCb)
    : ChannelInput(root, name, msgRecvCb, bufferRecvCb)
{
    try
    {
        _socketMessageIn = std::make_unique<zmq::socket_t>(_context, ZMQ_SUB);
        _socketBufferIn = std::make_unique<zmq::socket_t>(_context, ZMQ_SUB);
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
    }

    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
    _pathPrefix = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");

    _continueListening = true;
    _bufferInThread = std::thread([&]() { handleInputBuffers(); });
    _messageInThread = std::thread([&]() { handleInputMessages(); });
}

/*************/
ChannelInput_ZMQ::~ChannelInput_ZMQ()
{
    _continueListening = false;
    _messageInThread.join();
    _bufferInThread.join();
}

/*************/
void ChannelInput_ZMQ::handleInputMessages()
{
    try
    {
        // We don't want to miss a message: set the high water mark to a high value
        int hwm = 1000;
        _socketMessageIn->set(zmq::sockopt::rcvhwm, hwm);

        _socketMessageIn->bind((_pathPrefix + "msg_" + _name).c_str());
        _socketMessageIn->set(zmq::sockopt::subscribe, ""); // We subscribe to all incoming messages

        // Helper function to receive messages
        zmq::message_t msg;
        std::function<Values(void)> recvMessage;
        recvMessage = [&]() -> Values {
            auto result = _socketMessageIn->recv(msg, zmq::recv_flags::none); // size of the message
            if (!result)
                return Values();
            size_t size = *(static_cast<int*>(msg.data()));

            Values values;
            for (size_t i = 0; i < size; ++i)
            {
                if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                    return Values();
                Value::Type valueType = *(Value::Type*)msg.data();

                if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                    return Values();
                std::string valueName(static_cast<char*>(msg.data()));

                if (valueType == Value::Type::values)
                {
                    values.push_back(recvMessage());
                }
                else
                {
                    if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                        return Values();

                    if (valueType == Value::Type::boolean)
                        values.push_back(*(bool*)msg.data());
                    else if (valueType == Value::Type::integer)
                        values.push_back(*(int64_t*)msg.data());
                    else if (valueType == Value::Type::real)
                        values.push_back(*(double*)msg.data());
                    else if (valueType == Value::Type::string)
                        values.push_back(std::string((char*)msg.data()));
                }

                if (!valueName.empty())
                    values.back().setName(valueName);
            }
            return values;
        };

        while (_continueListening)
        {
            if (!_socketMessageIn->recv(msg, zmq::recv_flags::none)) // name of the target
                continue;

            std::string name((char*)msg.data());
            if (!_socketMessageIn->recv(msg, zmq::recv_flags::none)) // target's attribute
                return;

            std::string attribute((char*)msg.data());

            Values values = recvMessage();

            _msgRecvCb(name, attribute, values);
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
        // We only keep one buffer in memory while processing
        int highWaterMark = 1;
        _socketBufferIn->set(zmq::sockopt::rcvhwm, highWaterMark);

        _socketBufferIn->bind(_pathPrefix + "buf_" + _name);
        _socketBufferIn->set(zmq::sockopt::subscribe, ""); // We subscribe to all incoming messages

        while (_continueListening)
        {
            zmq::message_t msg;

            if (!_socketBufferIn->recv(msg, zmq::recv_flags::none))
                continue;

            std::string name(reinterpret_cast<char*>(msg.data()));

            if (!_socketBufferIn->recv(msg, zmq::recv_flags::none))
                continue;

            auto buffer = SerializedObject(static_cast<uint8_t*>(msg.data()), static_cast<uint8_t*>(msg.data()) + msg.size());
            _bufferRecvCb(name, std::move(buffer));
        }
    }
    catch (const zmq::error_t& error)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "ChannelInput_ZMQ::" << __FUNCTION__ << " - Exception: " << error.what() << Log::endl;
    }
}

} // namespace Splash
