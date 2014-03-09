#include "link.h"
#include "log.h"
#include "timer.h"

#define SPLASH_LINK_END_MSG "__end_msg"

using namespace std;

namespace Splash {

/*************/
Link::Link(RootObjectWeakPtr root, string name)
{
    _rootObject = root;
    _name = name;
    _context.reset(new zmq::context_t(1));

    _socketMessageOut.reset(new zmq::socket_t(*_context, ZMQ_PUB));
    _socketMessageIn.reset(new zmq::socket_t(*_context, ZMQ_SUB));
    _socketBufferOut.reset(new zmq::socket_t(*_context, ZMQ_PUB));
    _socketBufferIn.reset(new zmq::socket_t(*_context, ZMQ_SUB));

    _bufferInThread = thread([&]() {
        handleInputBuffers();
    });

    _messageInThread = thread([&]() {
        handleInputMessages();
    });
}

/*************/
Link::~Link()
{
    int lingerValue = 0;
    _socketMessageOut->setsockopt(ZMQ_LINGER, &lingerValue, sizeof(lingerValue));
    _socketBufferOut->setsockopt(ZMQ_LINGER, &lingerValue, sizeof(lingerValue));

    _socketMessageOut.reset();
    _socketBufferOut.reset();

    _context.reset();
    _bufferInThread.join();
    _messageInThread.join();
}

/*************/
void Link::connectTo(string name)
{
    // TODO: for now, all connections are through IPC.
    _socketMessageOut->connect((string("ipc:///tmp/splash_msg_") + name).c_str());
    _socketBufferOut->connect((string("ipc:///tmp/splash_buf_") + name).c_str());

    // Set the high water mark to a low value for the buffer output
    int hwm = 10000;
    _socketMessageOut->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));

    // Set the high water mark to a low value for the buffer output
    hwm = 5;
    _socketBufferOut->setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));

    // Wait a bit for the connection to be up
    timespec nap {0, (long int)1e8};
    nanosleep(&nap, NULL);
}

/*************/
bool Link::sendBuffer(string name, SerializedObjectPtr buffer)
{
    // We pack the buffer and its name in a single msg
    zmq::message_t msg(name.size() + 1 + buffer->size());
    memcpy(msg.data(), (void*)name.c_str(), name.size() + 1);
    memcpy((char*)msg.data() + name.size() + 1, buffer->data(), buffer->size());
    _socketBufferOut->send(msg);

    return true;
}

/*************/
bool Link::sendMessage(string name, string attribute, vector<Value> message)
{
    // First we send the name of the target
    zmq::message_t msg(name.size() + 1);
    memcpy(msg.data(), (void*)name.c_str(), name.size() + 1);
    _socketMessageOut->send(msg);

    // And the target's attribute
    msg.rebuild(attribute.size() + 1);
    memcpy(msg.data(), (void*)attribute.c_str(), attribute.size() + 1);
    _socketMessageOut->send(msg);

    // Then, the size of the message
    int size = message.size();
    msg.rebuild(sizeof(size));
    memcpy(msg.data(), (void*)&size, sizeof(size));
    _socketMessageOut->send(msg);

    // And every message
    for (auto& v : message)
    {
        Value::Type valueType = v.getType();
        int valueSize = (valueType == Value::Type::s) ? v.size() + 1 : v.size();
        void* value = v.data();

        msg.rebuild(sizeof(valueType));
        memcpy(msg.data(), (void*)&valueType, sizeof(valueType));
        _socketMessageOut->send(msg);
        msg.rebuild(valueSize);
        memcpy(msg.data(), value, valueSize);
        _socketMessageOut->send(msg);
    }

    // We don't display broadcast messages, for visibility
#ifdef DEBUG
    if (name != SPLASH_ALL_PAIRS)
        SLog::log << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Sending message to " << name << "::" << attribute << Log::endl;
#endif

    return true;
}

/*************/
void Link::handleInputMessages()
{
    try
    {
        _socketMessageIn->bind((string("ipc:///tmp/splash_msg_") + _name).c_str());
        _socketMessageIn->setsockopt(ZMQ_SUBSCRIBE, NULL, 0); // We subscribe to all incoming messages

        while (true)
        {
            zmq::message_t msg;
            _socketMessageIn->recv(&msg); // name of the target
            string name((char*)msg.data());
            _socketMessageIn->recv(&msg); // target's attribute
            string attribute((char*)msg.data());
            _socketMessageIn->recv(&msg); // size of the message
            int size = *(int*)msg.data();

            vector<Value> values;
            for (int i = 0; i < size; ++i)
            {
                _socketMessageIn->recv(&msg);
                Value::Type valueType = *(Value::Type*)msg.data();
                _socketMessageIn->recv(&msg);
                if (valueType == Value::Type::i)
                    values.push_back(*(int*)msg.data());
                else if (valueType == Value::Type::f)
                    values.push_back(*(float*)msg.data());
                else if (valueType == Value::Type::s)
                    values.push_back(string((char*)msg.data()));
            }

            // We don't display broadcast messages, for visibility
#ifdef DEBUG
            if (name != SPLASH_ALL_PAIRS)
                SLog::log << Log::DEBUGGING << "Link::" << __FUNCTION__ << " - Receiving message for " << name << "::" << attribute << Log::endl;
#endif

            auto root = _rootObject.lock();
            root->set(name, attribute, values);
        }
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            cout << "Exception: " << e.what() << endl;
    }


    _socketMessageIn.reset();
}

/*************/
void Link::handleInputBuffers()
{
    try
    {
        _socketBufferIn->bind((string("ipc:///tmp/splash_buf_") + _name).c_str());
        _socketBufferIn->setsockopt(ZMQ_SUBSCRIBE, NULL, 0); // We subscribe to all incoming messages

        // Set the high water mark to a low value for the buffer output
        int hwm = 5;
        _socketBufferIn->setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));

        while (true)
        {
            zmq::message_t msg;

            _socketBufferIn->recv(&msg);
            string name((char*)msg.data());
            SerializedObjectPtr buffer(new SerializedObject((char*)msg.data() + name.size() + 1, (char*)msg.data() + msg.size()));
            
            auto root = _rootObject.lock();
            root->setFromSerializedObject(name, buffer);
        }
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            cout << "Exception: " << e.what() << endl;
    }

    _socketBufferIn.reset();
}

} // end of namespace
