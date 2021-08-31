#include "./core/link.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./core/constants.h"
#include "./core/root_object.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Link::Link(RootObject* root, const std::string& name)
{
    try
    {
        _rootObject = root;
        _name = name;
        _context = std::make_unique<zmq::context_t>(2);

        _socketMessageOut = std::make_unique<zmq::socket_t>(*_context, ZMQ_PUB);
        _socketMessageIn = std::make_unique<zmq::socket_t>(*_context, ZMQ_SUB);
        _socketBufferOut = std::make_unique<zmq::socket_t>(*_context, ZMQ_PUB);
        _socketBufferIn = std::make_unique<zmq::socket_t>(*_context, ZMQ_SUB);

        // High water mark set to zero for the outputs
        int hwm = 0;
        _socketMessageOut->set(zmq::sockopt::sndhwm, hwm);
        _socketBufferOut->set(zmq::sockopt::sndhwm, hwm);
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }

    auto socketPrefix = _rootObject->getSocketPrefix();
    _basePath = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _basePath += socketPrefix + std::string("_");

    _running = true;
    _bufferInThread = std::thread([&]() { handleInputBuffers(); });
    _messageInThread = std::thread([&]() { handleInputMessages(); });
}

/*************/
Link::~Link()
{
    _running = false;
    _bufferInThread.join();
    _messageInThread.join();

    int lingerValue = 0;
    try
    {
        _socketMessageOut->set(zmq::sockopt::linger, lingerValue);
        _socketBufferOut->set(zmq::sockopt::linger, lingerValue);
    }
    catch (zmq::error_t& e)
    {
        Log::get() << Log::ERROR << "Link::" << __FUNCTION__ << " - Error while closing socket: " << e.what() << Log::endl;
    }
}

/*************/
void Link::connectTo(const std::string& name)
{
    if (find(_connectedTargets.begin(), _connectedTargets.end(), name) != _connectedTargets.end())
        return;

    _connectedTargets.push_back(name);

    try
    {
        // For now, all connections are through IPC.
        _socketMessageOut->connect((_basePath + "msg_" + name).c_str());
        _socketBufferOut->connect((_basePath + "buf_" + name).c_str());
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }

    // Wait a bit for the connection to be up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/*************/
void Link::disconnectFrom(const std::string& name)
{
    auto targetIt = find(_connectedTargets.begin(), _connectedTargets.end(), name);
    if (targetIt != _connectedTargets.end())
    {
        try
        {
            _socketMessageOut->disconnect((_basePath + "msg_" + name).c_str());
            _socketBufferOut->disconnect((_basePath + "buf_" + name).c_str());
            _connectedTargets.erase(targetIt);
        }
        catch (const zmq::error_t& e)
        {
            if (errno != ETERM)
                Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception while disconnecting from " << name << ": " << e.what() << Log::endl;
        }
    }
}

/*************/
bool Link::waitForBufferSending(std::chrono::milliseconds maximumWait)
{
    std::unique_lock<std::mutex> lock(_bufferTransmittedMutex);

    // If no buffer is currently being sent, return now
    if (_otgBufferCount == 0)
        return true;

    const auto startTime = Timer::getTime() / 1000; // Time in ms
    auto waitDurationLeft = maximumWait.count();
    while (true)
    {
        const auto status = _bufferTransmittedCondition.wait_for(lock, std::chrono::milliseconds(waitDurationLeft));
        if (status == std::cv_status::timeout)
            return false;
        else if (_otgBufferCount == 0)
            return true;
        waitDurationLeft = Timer::getTime() / 1000 - startTime;
    }

    return false;
}

/*************/
bool Link::sendBuffer(const std::string& name, std::shared_ptr<SerializedObject> buffer)
{
    try
    {
        std::lock_guard<Spinlock> lock(_bufferSendMutex);
        auto bufferPtr = buffer.get();

        _otgMutex.lock();
        _otgBuffers.push_back(buffer);
        _otgMutex.unlock();

        _otgBufferCount++;

        zmq::message_t msg(name.size() + 1);
        memcpy(msg.data(), (void*)name.c_str(), name.size() + 1);
        _socketBufferOut->send(msg, zmq::send_flags::sndmore);

        msg.rebuild(bufferPtr->data(), bufferPtr->size(), Link::freeSerializedBuffer, this);
        _socketBufferOut->send(msg, zmq::send_flags::none);
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }

    return true;
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
        sendMessage(message);
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }

// We don't display broadcast messages, for visibility
#ifdef DEBUG
    if (name != Constants::ALL_PEERS)
        Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Sending message to " << name << "::" << attribute << Log::endl;
#endif

    return true;
}

/*************/
void Link::freeSerializedBuffer(void* data, void* hint)
{
    Link* ctx = (Link*)hint;

    std::unique_lock<std::mutex> lock(ctx->_bufferTransmittedMutex);

    uint32_t index = 0;
    for (; index < ctx->_otgBuffers.size(); ++index)
        if (ctx->_otgBuffers[index]->data() == data)
            break;

    if (index >= ctx->_otgBuffers.size())
    {
        Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Buffer to free not found in currently sent buffers list" << Log::endl;
        return;
    }
    ctx->_otgBuffers.erase(ctx->_otgBuffers.begin() + index);
    ctx->_otgBufferCount--;
    
    ctx->_bufferTransmittedCondition.notify_all();
}

/*************/
void Link::handleInputMessages()
{
    try
    {
        // We don't want to miss a message: set the high water mark to a high value
        int hwm = 1000;
        _socketMessageIn->set(zmq::sockopt::rcvhwm, hwm);

        _socketMessageIn->bind((_basePath + "msg_" + _name).c_str());
        _socketMessageIn->set(zmq::sockopt::subscribe, ""); // We subscribe to all incoming messages

        // Helper function to receive messages
        zmq::message_t msg;
        std::function<Values(void)> recvMessage;
        recvMessage = [&]() -> Values {
            auto result = _socketMessageIn->recv(msg, zmq::recv_flags::none); // size of the message
            if (!result)
                return {};
            size_t size = *(static_cast<int*>(msg.data()));

            Values values;
            for (size_t i = 0; i < size; ++i)
            {
                if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                    return {};
                Value::Type valueType = *(Value::Type*)msg.data();

                if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                    return {};
                std::string valueName(static_cast<char*>(msg.data()));

                if (valueType == Value::Type::values)
                {
                    values.push_back(recvMessage());
                }
                else
                {
                    if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                        return {};

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

        while (_running)
        {
            if (!_socketMessageIn->recv(msg, zmq::recv_flags::dontwait)) // name of the target
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            std::string name((char*)msg.data());
            if (!_socketMessageIn->recv(msg, zmq::recv_flags::none)) // target's attribute
                return;

            std::string attribute((char*)msg.data());

            Values values = recvMessage();

            if (_rootObject)
                _rootObject->set(name, attribute, values);
// We don't display broadcast messages, for visibility
#ifdef DEBUG
            if (name != Constants::ALL_PEERS)
                Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " (" << _rootObject->getName() << ")"
                           << " - Receiving message for " << name << "::" << attribute << Log::endl;
#endif
        }
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }
}

/*************/
void Link::handleInputBuffers()
{
    try
    {
        // We only keep one buffer in memory while processing
        int hwm = 1;
        _socketBufferIn->set(zmq::sockopt::rcvhwm, hwm);

        _socketBufferIn->bind((_basePath + "buf_" + _name).c_str());
        _socketBufferIn->set(zmq::sockopt::subscribe, ""); // We subscribe to all incoming messages

        while (_running)
        {
            zmq::message_t msg;

            if (!_socketBufferIn->recv(msg, zmq::recv_flags::dontwait))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            std::string name((char*)msg.data());

            if (!_socketBufferIn->recv(msg, zmq::recv_flags::none))
                continue;
            auto buffer = std::make_shared<SerializedObject>(static_cast<uint8_t*>(msg.data()), static_cast<uint8_t*>(msg.data()) + msg.size());

            if (_rootObject)
                _rootObject->setFromSerializedObject(name, buffer);
        }
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }
}

} // namespace Splash
