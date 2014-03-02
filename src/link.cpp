#include "link.h"

#define SPLASH_LINK_END_MSG "__end_msg"

using namespace std;

namespace Splash {

/*************/
Link::Link(RootObjectWeakPtr root, string name)
{
    _rootObject = root;
    _name = name;
    _context.reset(new zmq::context_t(1));

    _socketMessageOut.reset(new zmq::socket_t(*_context, ZMQ_PUSH));
    _socketMessageIn.reset(new zmq::socket_t(*_context, ZMQ_PULL));
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
    _socketMessageOut->bind((string("ipc:///tmp/splash_msg_") + name).c_str());
    _socketBufferOut->bind((string("ipc:///tmp/splash_buf_") + name).c_str());
}

/*************/
bool Link::sendBuffer(string name, SerializedObjectPtr buffer)
{
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

    return true;
}

/*************/
void Link::handleInputMessages()
{
    try
    {
        _socketMessageIn->connect((string("ipc:///tmp/splash_msg_") + _name).c_str());

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
        _socketBufferIn->connect((string("ipc:///tmp/splash_buf_") + _name).c_str());
    }
    catch (const zmq::error_t& e)
    {
        if (errno != ETERM)
            cout << "Exception: " << e.what() << endl;
    }

    _socketBufferIn.reset();
}

} // end of namespace
