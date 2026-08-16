#ifndef HTTP_SERVER_INCLUDED
#define HTTP_SERVER_INCLUDED

#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string_view>
#include <memory>
#include <mutex>
#include <sstream>
#include <type_traits>

#include "logging.h"
#include "http_connection.h"
#include "net_constants.h"
#include "uuid.h"

namespace http {

class HttpServer {
public:
    HttpServer(const Router& router) : HttpServer(router, std::make_shared<logrr::ConsoleSink>()) {}
    HttpServer(const Router& router, std::shared_ptr<logrr::ILogSink> logsink);
    virtual ~HttpServer();
    HttpServer(HttpServer&& serv) noexcept;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(HttpServer&& serv) noexcept;
    HttpServer& operator=(const HttpServer&) = delete;

    bool listen();
    bool listen(
        const char* host, 
        const char* port=kHttpPort, 
        int max_connections=kMaxConnections, 
        int bufsize=kReceptionBufSize
    );
    bool continueListen();
    void stopListen() noexcept;
    bool addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept;
protected:
    bool buildSocket(const char* host, const char* port) noexcept;
    void freeAddrInfo(addrinfo*& servinfo) noexcept;
    void closeConnection(int& sockfd) noexcept;
    template <typename... Opts> bool setSockOptions(int sockfd, Opts&&... args) noexcept;
    template <typename Opt> bool applyOption(int sockfd, Opt&& arg, int opt) noexcept;
    bool listenInternal(int max_connections, int bufsize) noexcept;
    virtual void clientIntakeCycle(int bufsize) noexcept;
    ClientConnection acceptConnection() noexcept;
    void swap(HttpServer& other) noexcept;
protected:
    int sockfd_ = kEmptyDescriptor;
    addrinfo* servinfo_ = nullptr;
    addrinfo hints_;
    std::shared_ptr<logrr::Logger> logger_ = nullptr;
    bool is_running_ = false;
    std::mutex mtx_;
    Router router_;
};

inline bool HttpServer::listen() { return listen("0.0.0.0"); }
inline void HttpServer::stopListen() noexcept { closeConnection(sockfd_); }

} // namespace http

#endif