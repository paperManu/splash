#include "./core/link.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./core/root_object.h"
#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
Link::Link(RootObject* root, const string& name)
{
    try
    {
        _rootObject = root;
        _name = name;
        _context = make_unique<zmq::context_t>(2);

        _socketMessageOut = make_unique<zmq::socket_t>(*_context, ZMQ_PUB);
        _socketMessageIn = make_unique<zmq::socket_t>(*_context, ZMQ_SUB);
        _socketBufferOut = make_unique<zmq::socket_t>(*_context, ZMQ_PUB);
        _socketBufferIn = make_unique<zmq::socket_t>(*_context, ZMQ_SUB);

        // High water mark set to zero for the outputs
        int hwm = 0;
        _socketMessageOut->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
        _socketBufferOut->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
    }

    auto socketPrefix = _rootObject->getSocketPrefix();
    _basePath = "ipc:///tmp/splash_";
    if (!socketPrefix.empty())
        _basePath += socketPrefix + string("_");

    _running = true;
    _bufferInThread = thread([&]() { handleInputBuffers(); });
    _messageInThread = thread([&]() { handleInputMessages(); });
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
        _socketMessageOut->setsockopt(ZMQ_LINGER, &lingerValue, sizeof(lingerValue));
        _socketBufferOut->setsockopt(ZMQ_LINGER, &lingerValue, sizeof(lingerValue));
    }
    catch (zmq::error_t &e)
    {
        Log::get() << Log::ERROR << "Link::" << __FUNCTION__ << " - Error while closing socket: " << e.what() << Log::endl;
    }
}

/*************/
void Link::connectTo(const string& name)
{
    if (find(_connectedTargets.begin(), _connectedTargets.end(), name) == _connectedTargets.end())
        _connectedTargets.push_back(name);
    else
        return;

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
    this_thread::sleep_for(chrono::milliseconds(100));
    _connectedToOuter = true;
}

/*************/
void Link::connectTo(const std::string& name, RootObject* peer)
{
    if (!peer)
        return;

    auto rootObjectIt = _connectedTargetPointers.find(name);
    if (rootObjectIt == _connectedTargetPointers.end())
        _connectedTargetPointers[name] = peer;
    else
        return;

    // Wait a bit for the connection to be up
    this_thread::sleep_for(chrono::milliseconds(100));
    _connectedToInner = true;
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

    auto targetPointerIt = _connectedTargetPointers.find(name);
    if (targetPointerIt != _connectedTargetPointers.end())
    {
        _connectedTargetPointers.erase(targetPointerIt);
    }
}

/*************/
bool Link::waitForBufferSending(chrono::milliseconds maximumWait)
{
    bool returnValue;
    chrono::milliseconds totalWait{0};

    while (true)
    {
        if (_otgNumber == 0)
        {
            returnValue = true;
            break;
        }
        if (totalWait > maximumWait)
        {
            returnValue = false;
            break;
        }
        totalWait = totalWait + chrono::milliseconds(1);
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    return returnValue;
}

/*************/
bool Link::sendBuffer(const string& name, shared_ptr<SerializedObject> buffer)
{
    if (_connectedToInner)
    {
        for (auto& rootObjectIt : _connectedTargetPointers)
        {
            auto rootObject = rootObjectIt.second;
            // If there is also a connection to another process,
            // we make a copy of the buffer right now
            if (rootObject && _connectedToOuter)
            {
                auto copiedBuffer = make_shared<SerializedObject>();
                *copiedBuffer = *buffer;
                rootObject->setFromSerializedObject(name, copiedBuffer);
            }
            else if (rootObject)
            {
                rootObject->setFromSerializedObject(name, buffer);
            }
        }
    }

    if (_connectedToOuter)
    {
        try
        {
            lock_guard<Spinlock> lock(_bufferSendMutex);
            auto bufferPtr = buffer.get();

            _otgMutex.lock();
            _otgBuffers.push_back(buffer);
            _otgMutex.unlock();

            _otgNumber.fetch_add(1, std::memory_order_acq_rel);

            zmq::message_t msg(name.size() + 1);
            memcpy(msg.data(), (void*)name.c_str(), name.size() + 1);
            _socketBufferOut->send(msg, zmq::send_flags::sndmore);

            msg.rebuild(bufferPtr->data(), bufferPtr->size(), Link::freeOlderBuffer, this);
            _socketBufferOut->send(msg, zmq::send_flags::none);
        }
        catch (const zmq::error_t& e)
        {
            if (errno != ETERM)
                Log::get() << Log::WARNING << "Link::" << __FUNCTION__ << " - Exception: " << e.what() << Log::endl;
        }
    }

    return true;
}

/*************/
bool Link::sendBuffer(const string& name, const shared_ptr<BufferObject>& object)
{
    auto buffer = object->serialize();
    return sendBuffer(name, std::move(buffer));
}

/*************/
bool Link::sendMessage(const string& name, const string& attribute, const Values& message)
{
    if (_connectedToInner)
    {
        for (auto& rootObjectIt : _connectedTargetPointers)
        {
            auto rootObject = rootObjectIt.second;
            if (rootObject)
                rootObject->set(name, attribute, message);
        }
    }

    if (_connectedToOuter)
    {
        try
        {
            lock_guard<Spinlock> lock(_msgSendMutex);

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
    }

// We don't display broadcast messages, for visibility
#ifdef DEBUG
    if (name != SPLASH_ALL_PEERS)
        Log::get() << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Sending message to " << name << "::" << attribute << Log::endl;
#endif

    return true;
}

/*************/
void Link::freeOlderBuffer(void* data, void* hint)
{
    Link* ctx = (Link*)hint;
    lock_guard<Spinlock> lock(ctx->_otgMutex);
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
    ctx->_otgNumber.fetch_sub(1, std::memory_order_acq_rel);
}

/*************/
void Link::handleInputMessages()
{
    try
    {
        // We don't want to miss a message: set the high water mark to a high value
        int hwm = 1000;
        _socketMessageIn->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));

        _socketMessageIn->bind((_basePath + "msg_" + _name).c_str());
        _socketMessageIn->setsockopt(ZMQ_SUBSCRIBE, NULL, 0); // We subscribe to all incoming messages

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
                string valueName(static_cast<char*>(msg.data()));

                if (valueType == Value::Type::values)
                {
                    values.push_back(recvMessage());
                }
                else
                {
                    if (!_socketMessageIn->recv(msg, zmq::recv_flags::none))
                        return {};

                    if (valueType == Value::Type::integer)
                        values.push_back(*(int64_t*)msg.data());
                    else if (valueType == Value::Type::real)
                        values.push_back(*(double*)msg.data());
                    else if (valueType == Value::Type::string)
                        values.push_back(string((char*)msg.data()));
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
                std::this_thread::sleep_for(1ms);
                continue;
            }
            string name((char*)msg.data());
            if (!_socketMessageIn->recv(msg, zmq::recv_flags::none)) // target's attribute
                return;

            string attribute((char*)msg.data());

            Values values = recvMessage();

            if (_rootObject)
                _rootObject->set(name, attribute, values);
// We don't display broadcast messages, for visibility
#ifdef DEBUG
            if (name != SPLASH_ALL_PEERS)
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
        _socketBufferIn->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));

        _socketBufferIn->bind((_basePath + "buf_" + _name).c_str());
        _socketBufferIn->setsockopt(ZMQ_SUBSCRIBE, NULL, 0); // We subscribe to all incoming messages

        while (_running)
        {
            zmq::message_t msg;

            if (!_socketBufferIn->recv(msg, zmq::recv_flags::dontwait))
            {
                std::this_thread::sleep_for(1ms);
                continue;
            }
            string name((char*)msg.data());

            if (!_socketBufferIn->recv(msg, zmq::recv_flags::none))
                continue;
            auto buffer = make_shared<SerializedObject>(static_cast<uint8_t*>(msg.data()), static_cast<uint8_t*>(msg.data()) + msg.size());

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

} // end of namespace
