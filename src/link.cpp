#include "link.h"

using namespace std;

namespace Splash {

/*************/
Link::Link()
{
    _context.reset(new zmq::context_t(0));
    _socketMessageOut.reset(new zmq::socket_t(*_context, ZMQ_PUSH));
    _socketMessageIn.reset(new zmq::socket_t(*_context, ZMQ_PULL));
    _socketBufferOut.reset(new zmq::socket_t(*_context, ZMQ_PUB));
    _socketBufferIn.reset(new zmq::socket_t(*_context, ZMQ_SUB));
}

/*************/
Link::~Link()
{
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
bool Link::sendMessage(string name, vector<Value> message)
{
    return true;
}

/*************/
void Link::handleInputMessages()
{
}

/*************/
void Link::handleInputBuffers()
{
}

} // end of namespace
