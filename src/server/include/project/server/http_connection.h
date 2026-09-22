#ifndef HTTP_CONNECTION_INCLUDED
#define HTTP_CONNECTION_INCLUDED

#include <sys/socket.h>
#include <unistd.h>
#include <random>
#include <sstream>
#include <iomanip>

#include "project/server/http_codec.h"
#include "project/common/http_message.h"
#include "project/logging/logging.h"
#include "project/common/net_constants.h"


namespace http {

class ClientConnection final
{
public:
    std::string id;
    int sockfd = kInvalidSocket;
    std::string ip = "";
    uint16_t port = 0;
    
    ClientConnection() = default;
    ClientConnection(
        std::string_view id_, 
        int sockfd_,
        std::string ip_,
        uint16_t port_
    ) : id(id_), sockfd(sockfd_), ip(ip_), port(port_) {}

    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;

    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;

    ~ClientConnection() {
        if (sockfd > 0) {
            close(sockfd);
        }
    }
};


class HttpConnection {
public:
    HttpConnection(
        ClientConnection&& client, 
        int bufsize=kReceptionBufSize
    );
    virtual ~HttpConnection();
    virtual bool process();
protected:
    virtual bool recv() noexcept;
    virtual bool send(const std::string&) noexcept;
    std::string execution() noexcept;
    void closeConnection(int& sockfd) noexcept;
protected:
    ClientConnection client_; 
    std::string req_;
    int bufsize_;
};

} // namespace http

#endif