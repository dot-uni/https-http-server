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


namespace {
    void* getSinAddr(sockaddr *sa) 
    {
        if (sa->sa_family == AF_INET) {
            return &(((sockaddr_in*)sa)->sin_addr);
        }
        else if (sa->sa_family == AF_INET6) {
            return &(((sockaddr_in6*)sa)->sin6_addr);
        }
        return nullptr;
    }

    uint16_t getSinPort(sockaddr *sa) 
    {
        if (sa->sa_family == AF_INET) {
            return (((sockaddr_in*)sa)->sin_port);
        }
        else if (sa->sa_family == AF_INET6) {
            return (((sockaddr_in6*)sa)->sin6_port);
        }
        return 0;
    }

    std::string getIpAddr(sockaddr* addr) 
    {
        char str_addr[INET6_ADDRSTRLEN];
        const char* success;

        success = inet_ntop(addr->sa_family, getSinAddr(addr), str_addr, sizeof(str_addr));
        if (success == nullptr) {
            return "";
        }
        return std::string(str_addr);
    }
} // namespace


namespace http {


class HttpServer 
{
public:
    explicit HttpServer();
    virtual ~HttpServer();
    HttpServer(HttpServer&& serv) = delete;
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(HttpServer&& serv) = delete;
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
    bool is_running_ = false;
    std::mutex mtx_;
};

inline bool HttpServer::listen() { return listen("0.0.0.0"); }
inline void HttpServer::stopListen() noexcept { closeConnection(sockfd_); }

} // namespace http

#endif