#include "httpServer.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <signal.h>

#include "log.h"
#include "scene.h"

using namespace std;

namespace Splash {

namespace Http {
    
    /*************/
    /*************/
    namespace status_strings
    {
        const std::string ok =
          "HTTP/1.0 200 OK\r\n";
        const std::string created =
          "HTTP/1.0 201 Created\r\n";
        const std::string accepted =
          "HTTP/1.0 202 Accepted\r\n";
        const std::string no_content =
          "HTTP/1.0 204 No Content\r\n";
        const std::string multiple_choices =
          "HTTP/1.0 300 Multiple Choices\r\n";
        const std::string moved_permanently =
          "HTTP/1.0 301 Moved Permanently\r\n";
        const std::string moved_temporarily =
          "HTTP/1.0 302 Moved Temporarily\r\n";
        const std::string not_modified =
          "HTTP/1.0 304 Not Modified\r\n";
        const std::string bad_request =
          "HTTP/1.0 400 Bad Request\r\n";
        const std::string unauthorized =
          "HTTP/1.0 401 Unauthorized\r\n";
        const std::string forbidden =
          "HTTP/1.0 403 Forbidden\r\n";
        const std::string not_found =
          "HTTP/1.0 404 Not Found\r\n";
        const std::string internal_server_error =
          "HTTP/1.0 500 Internal Server Error\r\n";
        const std::string not_implemented =
          "HTTP/1.0 501 Not Implemented\r\n";
        const std::string bad_gateway =
          "HTTP/1.0 502 Bad Gateway\r\n";
        const std::string service_unavailable =
          "HTTP/1.0 503 Service Unavailable\r\n";
        
        boost::asio::const_buffer to_buffer(Reply::StatusType status)
        {
          switch (status)
          {
          case Reply::ok:
            return boost::asio::buffer(ok);
          case Reply::created:
            return boost::asio::buffer(created);
          case Reply::accepted:
            return boost::asio::buffer(accepted);
          case Reply::no_content:
            return boost::asio::buffer(no_content);
          case Reply::multiple_choices:
            return boost::asio::buffer(multiple_choices);
          case Reply::moved_permanently:
            return boost::asio::buffer(moved_permanently);
          case Reply::moved_temporarily:
            return boost::asio::buffer(moved_temporarily);
          case Reply::not_modified:
            return boost::asio::buffer(not_modified);
          case Reply::bad_request:
            return boost::asio::buffer(bad_request);
          case Reply::unauthorized:
            return boost::asio::buffer(unauthorized);
          case Reply::forbidden:
            return boost::asio::buffer(forbidden);
          case Reply::not_found:
            return boost::asio::buffer(not_found);
          case Reply::internal_server_error:
            return boost::asio::buffer(internal_server_error);
          case Reply::not_implemented:
            return boost::asio::buffer(not_implemented);
          case Reply::bad_gateway:
            return boost::asio::buffer(bad_gateway);
          case Reply::service_unavailable:
            return boost::asio::buffer(service_unavailable);
          default:
            return boost::asio::buffer(internal_server_error);
          }
        }
    
    } // namespace status_strings
    
    namespace misc_strings {
    
    const char name_value_separator[] = { ':', ' ' };
    const char crlf[] = { '\r', '\n' };
    
    } // namespace misc_strings
    
    std::vector<boost::asio::const_buffer> Reply::toBuffers()
    {
      std::vector<boost::asio::const_buffer> buffers;
      buffers.push_back(status_strings::to_buffer(status));
      for (std::size_t i = 0; i < headers.size(); ++i)
      {
        Header& h = headers[i];
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(misc_strings::crlf));
      }
      buffers.push_back(boost::asio::buffer(misc_strings::crlf));
      buffers.push_back(boost::asio::buffer(content));
      return buffers;
    }
    
    namespace stock_replies
    {
        const char ok[] = "";
        const char created[] =
            "<html>"
            "<head><title>Created</title></head>"
            "<body><h1>201 Created</h1></body>"
            "</html>";
        const char accepted[] =
            "<html>"
            "<head><title>Accepted</title></head>"
            "<body><h1>202 Accepted</h1></body>"
            "</html>";
        const char no_content[] =
            "<html>"
            "<head><title>No Content</title></head>"
            "<body><h1>204 Content</h1></body>"
            "</html>";
        const char multiple_choices[] =
            "<html>"
            "<head><title>Multiple Choices</title></head>"
            "<body><h1>300 Multiple Choices</h1></body>"
            "</html>";
        const char moved_permanently[] =
            "<html>"
            "<head><title>Moved Permanently</title></head>"
            "<body><h1>301 Moved Permanently</h1></body>"
            "</html>";
        const char moved_temporarily[] =
            "<html>"
            "<head><title>Moved Temporarily</title></head>"
            "<body><h1>302 Moved Temporarily</h1></body>"
            "</html>";
        const char not_modified[] =
            "<html>"
            "<head><title>Not Modified</title></head>"
            "<body><h1>304 Not Modified</h1></body>"
            "</html>";
        const char bad_request[] =
            "<html>"
            "<head><title>Bad Request</title></head>"
            "<body><h1>400 Bad Request</h1></body>"
            "</html>";
        const char unauthorized[] =
            "<html>"
            "<head><title>Unauthorized</title></head>"
            "<body><h1>401 Unauthorized</h1></body>"
            "</html>";
        const char forbidden[] =
            "<html>"
            "<head><title>Forbidden</title></head>"
            "<body><h1>403 Forbidden</h1></body>"
            "</html>";
        const char not_found[] =
            "<html>"
            "<head><title>Not Found</title></head>"
            "<body><h1>404 Not Found</h1></body>"
            "</html>";
        const char internal_server_error[] =
            "<html>"
            "<head><title>Internal Server Error</title></head>"
            "<body><h1>500 Internal Server Error</h1></body>"
            "</html>";
        const char not_implemented[] =
            "<html>"
            "<head><title>Not Implemented</title></head>"
            "<body><h1>501 Not Implemented</h1></body>"
            "</html>";
        const char bad_gateway[] =
            "<html>"
            "<head><title>Bad Gateway</title></head>"
            "<body><h1>502 Bad Gateway</h1></body>"
            "</html>";
        const char service_unavailable[] =
            "<html>"
            "<head><title>Service Unavailable</title></head>"
            "<body><h1>503 Service Unavailable</h1></body>"
            "</html>";
        
        std::string to_string(Reply::StatusType status)
        {
            switch (status)
            {
            case Reply::ok:
                return ok;
            case Reply::created:
                return created;
            case Reply::accepted:
                return accepted;
            case Reply::no_content:
                return no_content;
            case Reply::multiple_choices:
                return multiple_choices;
            case Reply::moved_permanently:
                return moved_permanently;
            case Reply::moved_temporarily:
                return moved_temporarily;
            case Reply::not_modified:
                return not_modified;
            case Reply::bad_request:
                return bad_request;
            case Reply::unauthorized:
                return unauthorized;
            case Reply::forbidden:
                return forbidden;
            case Reply::not_found:
                return not_found;
            case Reply::internal_server_error:
                return internal_server_error;
            case Reply::not_implemented:
                return not_implemented;
            case Reply::bad_gateway:
                return bad_gateway;
            case Reply::service_unavailable:
                return service_unavailable;
            default:
                return internal_server_error;
            }
        }
    } // namespace stock_replies
    
    /*************/
    Reply Reply::stockReply(Reply::StatusType status)
    {
        Reply rep;
        rep.status = status;
        rep.content = stock_replies::to_string(status);
        rep.headers.resize(2);
        rep.headers[0].name = "Content-Length";
        rep.headers[0].value = std::to_string(rep.content.size());
        rep.headers[1].name = "Content-Type";
        rep.headers[1].value = "text/html";
        return rep;
    }
    
    /*************/
    /*************/
    RequestParser::RequestParser() :
        _state(method_start)
    {
    }
    
    /*************/
    void RequestParser::reset()
    {
        _state = method_start;
    }
    
    /*************/
    RequestParser::ResultType RequestParser::consume(Request& req, char input)
    {
        switch (_state)
        {
        case method_start:
          if (!is_char(input) || is_ctl(input) || is_tspecial(input))
          {
            return bad;
          }
          else
          {
            _state = method;
            req.method.push_back(input);
            return indeterminate;
          }
        case method:
          if (input == ' ')
          {
            _state = uri;
            return indeterminate;
          }
          else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
          {
            return bad;
          }
          else
          {
            req.method.push_back(input);
            return indeterminate;
          }
        case uri:
          if (input == ' ')
          {
            _state = http_version_h;
            return indeterminate;
          }
          else if (is_ctl(input))
          {
            return bad;
          }
          else
          {
            req.uri.push_back(input);
            return indeterminate;
          }
        case http_version_h:
          if (input == 'H')
          {
            _state = http_version_t_1;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_t_1:
          if (input == 'T')
          {
            _state = http_version_t_2;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_t_2:
          if (input == 'T')
          {
            _state = http_version_p;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_p:
          if (input == 'P')
          {
            _state = http_version_slash;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_slash:
          if (input == '/')
          {
            req.http_version_major = 0;
            req.http_version_minor = 0;
            _state = http_version_major_start;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_major_start:
          if (is_digit(input))
          {
            req.http_version_major = req.http_version_major * 10 + input - '0';
            _state = http_version_major;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_major:
          if (input == '.')
          {
            _state = http_version_minor_start;
            return indeterminate;
          }
          else if (is_digit(input))
          {
            req.http_version_major = req.http_version_major * 10 + input - '0';
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_minor_start:
          if (is_digit(input))
          {
            req.http_version_minor = req.http_version_minor * 10 + input - '0';
            _state = http_version_minor;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case http_version_minor:
          if (input == '\r')
          {
            _state = expecting_newline_1;
            return indeterminate;
          }
          else if (is_digit(input))
          {
            req.http_version_minor = req.http_version_minor * 10 + input - '0';
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case expecting_newline_1:
          if (input == '\n')
          {
            _state = header_line_start;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case header_line_start:
          if (input == '\r')
          {
            _state = expecting_newline_3;
            return indeterminate;
          }
          else if (!req.headers.empty() && (input == ' ' || input == '\t'))
          {
            _state = header_lws;
            return indeterminate;
          }
          else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
          {
            return bad;
          }
          else
          {
            req.headers.push_back(Header());
            req.headers.back().name.push_back(input);
            _state = header_name;
            return indeterminate;
          }
        case header_lws:
          if (input == '\r')
          {
            _state = expecting_newline_2;
            return indeterminate;
          }
          else if (input == ' ' || input == '\t')
          {
            return indeterminate;
          }
          else if (is_ctl(input))
          {
            return bad;
          }
          else
          {
            _state = header_value;
            req.headers.back().value.push_back(input);
            return indeterminate;
          }
        case header_name:
          if (input == ':')
          {
            _state = space_before_header_value;
            return indeterminate;
          }
          else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
          {
            return bad;
          }
          else
          {
            req.headers.back().name.push_back(input);
            return indeterminate;
          }
        case space_before_header_value:
          if (input == ' ')
          {
            _state = header_value;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case header_value:
          if (input == '\r')
          {
            _state = expecting_newline_2;
            return indeterminate;
          }
          else if (is_ctl(input))
          {
            return bad;
          }
          else
          {
            req.headers.back().value.push_back(input);
            return indeterminate;
          }
        case expecting_newline_2:
          if (input == '\n')
          {
            _state = header_line_start;
            return indeterminate;
          }
          else
          {
            return bad;
          }
        case expecting_newline_3:
          return (input == '\n') ? good : bad;
        default:
          return bad;
        }
    }
    
    /*************/
    bool RequestParser::is_char(int c)
    {
      return c >= 0 && c <= 127;
    }
    
    /*************/
    bool RequestParser::is_ctl(int c)
    {
      return (c >= 0 && c <= 31) || (c == 127);
    }
    
    /*************/
    bool RequestParser::is_tspecial(int c)
    {
      switch (c)
      {
      case '(': case ')': case '<': case '>': case '@':
      case ',': case ';': case ':': case '\\': case '"':
      case '/': case '[': case ']': case '?': case '=':
      case '{': case '}': case ' ': case '\t':
        return true;
      default:
        return false;
      }
    }
    
    /*************/
    bool RequestParser::is_digit(int c)
    {
      return c >= '0' && c <= '9';
    }
    
    /*************/
    /*************/
    Connection::Connection(boost::asio::ip::tcp::socket socket, ConnectionManager& manager, RequestHandler& handler) :
        _socket(std::move(socket)),
        _connectionManager(manager),
        _requestHandler(handler)
    {
        _buffer.resize(8192);
    }
    
    /*************/
    void Connection::start()
    {
        doRead();
    }
    
    /*************/
    void Connection::stop()
    {
        _socket.close();
    }
    
    /*************/
    void Connection::doRead()
    {
        auto self(shared_from_this());
        boost::system::error_code ec;
        auto bytesTransferred = _socket.read_some(boost::asio::buffer(_buffer), ec);

        if (!ec)
        {
            RequestParser::ResultType result;
            std::tie(result, std::ignore) = _requestParser.parse(_request, _buffer.data(), _buffer.data() + bytesTransferred);
    
            if (result == RequestParser::good)
            {
                _requestHandler.handleRequest(_request, _reply);
                doWrite();
            }
            else if (result == RequestParser::bad)
            {
                _reply = Reply::stockReply(Reply::bad_request);
                doWrite();
            }
            else
            {
                doRead();
            }
        }
        else if (ec != boost::asio::error::operation_aborted)
        {
            _connectionManager.stop(shared_from_this());
        }
    }
    
    /*************/
    void Connection::doWrite()
    {
        auto self(shared_from_this());
        boost::asio::async_write(_socket, _reply.toBuffers(), [this, self](boost::system::error_code ec, size_t) {
            if (!ec)
            {
                boost::system::error_code ignore_ec;
                _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_ec);
            }
    
            if (ec != boost::asio::error::operation_aborted)
            {
                _connectionManager.stop(shared_from_this());
            }
        });
    }
    
    /*************/
    /*************/
    void ConnectionManager::start(ConnectionPtr c)
    {
        _connections.insert(c);
        c->start();
    }
    
    /*************/
    void ConnectionManager::stop(ConnectionPtr c)
    {
        _connections.erase(c);
        c->stop();
    }
    
    /*************/
    void ConnectionManager::stopAll()
    {
        for (auto& c : _connections)
            c->stop();
         _connections.clear();
    }
    
    /*************/
    /*************/
    RequestHandler::RequestHandler()
    {
    }
    
    /*************/
    void RequestHandler::handleRequest(const Request& req, Reply& rep)
    {
        unique_lock<mutex> lock(_queueMutex);
    
        string requestPath;
        Values requestArgs;
        if (!urlDecode(req.uri, requestPath, requestArgs))
            return;
    
        atomic_bool replyState {false};
        string replyString {""};

        for (auto& cmdArgs : requestArgs)
        {
            auto args = cmdArgs.asValues();
            if (args.size() == 0)
                continue;

            if (args[0].asString() == "/set")
                _commandQueue.push_back({CommandId::set, args});
            else if (args[0].asString() == "/get")
                _commandQueue.push_back({CommandId::get, args});
            else if (args[0].asString() == "/scene")
                _commandQueue.push_back({CommandId::scene, args});
            else
            {
                Log::get() << Log::WARNING << "RequestHandler::" << __FUNCTION__ << " - No command associated to string " << args[0].asString() << Log::endl;
                rep = Reply::stockReply(Reply::bad_request);
                continue;
            }

            condition_variable cv;
            atomic_bool waiting {true};

            auto replyFunc = [&](string answer) -> void {
                if (!waiting)
                    return;

                replyState = true;
                replyString = answer;
                cv.notify_all();
            };

            _commandReturnFuncQueue.push_back(replyFunc);
            cv.wait_for(lock, chrono::milliseconds(2000));
            waiting = false;
        }
    
        if (replyState)
        {
            rep = Reply::stockReply(Reply::ok);
            rep.status = Reply::ok;
            rep.content = replyString.c_str();
            rep.headers.resize(3);
            rep.headers[0].name = "Content-Length";
            rep.headers[0].value = std::to_string(replyString.size());
            rep.headers[1].name = "Content-Type";
            rep.headers[1].value = "text";
            rep.headers[2].name = "Access-Control-Allow-Origin";
            rep.headers[2].value = "*";
        }
        else
        {
            rep = Reply::stockReply(Reply::internal_server_error);
        }
    }
    
    /*************/
    pair<RequestHandler::Command, RequestHandler::ReturnFunction> RequestHandler::getNextCommand()
    {
        lock_guard<mutex> lock(_queueMutex);
        if (_commandQueue.size() == 0)
            return make_pair(Command(CommandId::nop, Values()), ReturnFunction());
    
        auto command = _commandQueue[0];
        _commandQueue.pop_front();

        auto func = _commandReturnFuncQueue[0];
        _commandReturnFuncQueue.pop_front();

        return make_pair(command, func);
    }
    
    /*************/
    bool RequestHandler::urlDecode(const std::string& in, std::string& out, Values& args)
    {
        out.clear();
        out.reserve(in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
        {
            if (in[i] == '%')
            {
                if (i + 3 <= in.size())
                {
                    int value = 0;
                    std::istringstream is(in.substr(i + 1, 2));
                    if (is >> std::hex >> value)
                    {
                        out += static_cast<char>(value);
                        i += 2;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else if (in[i] == '+')
            {
              out += ' ';
            }
            else
            {
              out += in[i];
            }
        }
    
        // Get the command arguments
        // Different commands are separated by '&'
        std::string nextArg;
        auto currentCommandArgs = Values();
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            if (out[i] == '&')
            {
                currentCommandArgs.push_back(nextArg);
                nextArg = "/";
                args.push_back(currentCommandArgs);
                currentCommandArgs = Values();
            }
            else if (out[i] == '=' || out[i] == '!' || out[i] == '?')
            {
                if (nextArg != "")
                    currentCommandArgs.push_back(nextArg);
    
                nextArg.clear();
    
                continue;
            }
            else
            {
                nextArg.push_back(out[i]);
            }
    
            if (i == out.size() - 1 && nextArg != "")
            {
                currentCommandArgs.push_back(nextArg);
                args.push_back(currentCommandArgs);
            }
        }
        
        return true;
    }

} // end of namespace Http

/*************/
/*************/
HttpServer::HttpServer(const string& address, const string& port, SceneWeakPtr scene) :
    BaseObject(scene),
    _ioService(),
    _acceptor(_ioService),
    _socket(_ioService),
    _connectionManager()
{
    _scene = scene;

    registerAttributes();

    try
    {
        boost::asio::ip::tcp::resolver resolver(_ioService);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({address, port});
        _acceptor.open(endpoint.protocol());

        _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        _acceptor.bind(endpoint);
        _acceptor.listen();
    }
    catch (boost::system::system_error ec)
    {
        Log::get() << Log::ERROR << "HttpServer::" << __FUNCTION__ << " - Unable to open http server at " << address << ":" << port << Log::endl;
        _ready = false;
        return;
    }

    Log::get() << Log::MESSAGE << "HttpServer::" << __FUNCTION__ << " - Http server opened at " << address << ":" << port << Log::endl;
    _ready = true;
    _address = address;
    _port = port;

    doAccept();
}

/*************/
void HttpServer::run()
{
    _messageHandlerThread = std::thread([&]() {
        pair<Http::RequestHandler::Command, Http::RequestHandler::ReturnFunction> message;
        
        while (_ready)
        {
            if ((message = _requestHandler.getNextCommand()).first.command != Http::RequestHandler::CommandId::nop)
            {
                auto command = message.first.command;
                auto args = message.first.args;
                auto returnFunc = message.second;

                if (command == Http::RequestHandler::CommandId::set)
                {
                    if (args.size() < 4)
                        continue;

                    auto scene = _scene.lock();
                    auto objectName = args[1].asString();

                    for (int idx = 2; idx < args.size(); idx += 2)
                    {
                        auto attrName = args[idx].asString();
                        Values attrValue({args[idx + 1]});

                        // Local
                        if (scene->_objects.find(objectName) != scene->_objects.end())
                            scene->_objects[objectName]->setAttribute(attrName, attrValue);

                        // And global
                        Values sendValues {objectName, attrName};
                        for (auto& v : attrValue)
                            sendValues.push_back(v);
                        scene->sendMessageToWorld("sendAll", sendValues);
                    }

                    returnFunc("");
                }
                else if (command == Http::RequestHandler::CommandId::get)
                {
                    if (args.size() < 3)
                        continue;

                    auto scene = _scene.lock();
                    auto objectName = args[1].asString();
                    auto attrName = args[2].asString();

                    auto values = scene->getAttributeFromObject(objectName, attrName);

                    Json::Value jsValue;
                    jsValue[attrName] = getValuesAsJson(values);
                    string strValue = jsValue.toStyledString();
                    returnFunc(strValue);
                }
                else if (command == Http::RequestHandler::CommandId::scene)
                {
                    if (args.size() < 2)
                        continue;

                    auto scene = _scene.lock();
                    auto commandName = args[1].asString();
                    if (!scene->setAttribute(commandName, {}))
                        returnFunc("OK");
                    else
                        returnFunc("Failed");
                }
            }
            else
            {
                this_thread::sleep_for(chrono::milliseconds(5));
            }
        }
    });
    _ioService.run();
}

/*************/
void HttpServer::stop()
{
    _ready = false;
    if (_messageHandlerThread.joinable())
        _messageHandlerThread.join();

    _acceptor.close();
    _connectionManager.stopAll();
    _ioService.stop();
}

/*************/
void HttpServer::doAccept()
{
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
        if (!_acceptor.is_open())
            return;

        if (!ec)
            _connectionManager.start(make_shared<Http::Connection>(std::move(_socket), _connectionManager, _requestHandler));

        doAccept();
    });
}

/*************/
void HttpServer::registerAttributes()
{
}

} // end of namespace
