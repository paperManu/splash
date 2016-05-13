/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GifBox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GifBox.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @httpServer.h
 * The HttpServer class
 */

#ifndef SPLASH_HTTPSERVER_H
#define SPLASH_HTTPSERVER_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>

#include <boost/asio.hpp>

#include "coretypes.h"
#include "basetypes.h"

namespace Splash {

namespace Http {
    /*************/
    class ConnectionManager;
    class RequestHandler;
    
    /*************/
    struct Header
    {
        std::string name;
        std::string value;
    };
    
    /*************/
    struct Request
    {
        std::string method;
        std::string uri;
        int http_version_major;
        int http_version_minor;
        std::vector<Header> headers;
    };
    
    /*************/
    struct Reply
    {
        enum StatusType
        {
            ok = 200,
            created = 201,
            accepted = 202,
            no_content = 204,
            multiple_choices = 300,
            moved_permanently = 301,
            moved_temporarily = 302,
            not_modified = 304,
            bad_request = 400,
            unauthorized = 401,
            forbidden = 403,
            not_found = 404,
            internal_server_error = 500,
            not_implemented = 501,
            bad_gateway = 502,
            service_unavailable = 503
        } status;
    
        std::vector<Header> headers;
        std::string content;
    
        std::vector<boost::asio::const_buffer> toBuffers();
    
        static Reply stockReply(StatusType status);
    };
    
    /*************/
    class RequestParser
    {
        public:
            enum ResultType 
            {
                good,
                bad,
                indeterminate
            };
    
            RequestParser();
    
            void reset();
    
            template<typename InputIterator>
            std::tuple<ResultType, InputIterator> parse(Request& req, InputIterator begin, InputIterator end)
            {
                while (begin != end)
                {
                    ResultType result = consume(req, *begin++);
                    if (result == good || result == bad)
                        return std::make_tuple(result, begin);
                }
                return std::make_tuple(indeterminate, begin);
            }
    
        private:
            ResultType consume(Request& req, char input);
    
            static bool is_char(int c);
            static bool is_ctl(int c);
            static bool is_tspecial(int c);
            static bool is_digit(int c);
    
            enum state
            {
                method_start,
                method,
                uri,
                http_version_h,
                http_version_t_1,
                http_version_t_2,
                http_version_p,
                http_version_slash,
                http_version_major_start,
                http_version_major,
                http_version_minor_start,
                http_version_minor,
                expecting_newline_1,
                header_line_start,
                header_lws,
                header_name,
                space_before_header_value,
                header_value,
                expecting_newline_2,
                expecting_newline_3
            } _state;
    };
    
    /*************/
    class Connection : public std::enable_shared_from_this<Connection>
    {
        public:
            explicit Connection(boost::asio::ip::tcp::socket socket, ConnectionManager& manager, RequestHandler& handler);
            
            void start();
            void stop();
    
        private:
            boost::asio::ip::tcp::socket _socket;
            ConnectionManager& _connectionManager;
            RequestHandler& _requestHandler;
            RequestParser _requestParser;
            Request _request;
            Reply _reply;
    
            std::vector<char> _buffer;
    
            void doRead();
            void doWrite();
    };
    
    typedef std::shared_ptr<Connection> ConnectionPtr;
    
    /*************/
    class ConnectionManager
    {
        public:
            void start(ConnectionPtr c);
            void stop(ConnectionPtr c);
            void stopAll();
    
        private:
            std::set<ConnectionPtr> _connections;
    };
    
    /*************/
    class RequestHandler
    {
        public:
            enum CommandId
            {
                nop,
                get,
                set,
                scene
            };
    
            struct Command
            {
                Command() {}
                Command(CommandId cmd, Values a)
                {
                    command = cmd;
                    args = a;
                }
    
                CommandId command {CommandId::nop};
                Values args {};
            };
    
            using ReturnFunction = std::function<void(std::string)>;
    
            explicit RequestHandler();
            void handleRequest(const Request& req, Reply& rep);
            std::pair<Command, ReturnFunction> getNextCommand();
    
        private:
            static bool urlDecode(const std::string& in, std::string& out, Values& args);
            std::deque<Command> _commandQueue;
            std::deque<ReturnFunction> _commandReturnFuncQueue;
            std::mutex _queueMutex;
    };
} // end of namespace Http

/*************/
class Scene;
typedef std::weak_ptr<Scene> SceneWeakPtr;

class HttpServer : public BaseObject
{
    public:
        HttpServer(const std::string& address, const std::string& port, SceneWeakPtr scene);

        explicit operator bool() const
        {
            return _ready;
        }

        void run();
        void stop();

        std::string getAddress() const {return _address;}
        std::string getPort() const {return _port;}

    private:
        SceneWeakPtr _scene;
        bool _ready {false};
        std::string _address {""};
        std::string _port {""};

        boost::asio::io_service _ioService;
        boost::asio::ip::tcp::acceptor _acceptor;
        boost::asio::ip::tcp::socket _socket;
        Http::RequestHandler _requestHandler;

        Http::ConnectionManager _connectionManager;
        std::thread _messageHandlerThread;

        void doAccept();

        void registerAttributes();
};

typedef std::shared_ptr<HttpServer> HttpServerPtr;

} // end of namespace

#endif
