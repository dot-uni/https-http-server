#ifndef HTTP_CONNECTION_INCLUDED
#define HTTP_CONNECTION_INCLUDED

#include <sys/socket.h>
#include <unistd.h>
#include <random>
#include <sstream>
#include <iomanip>

#include "status_logging.h"
#include "http_codec.h"
#include "http_message.h"
#include "status.h"
#include "ret_status.h"
#include "net_constants.h"


namespace http {

struct ClientConnection 
{
    std::string id;
    int sockfd = kInvalidSocket;
    std::string ip = "";
    uint16_t port = 0;
};


class HttpConnection {
public:
    HttpConnection(
        ClientConnection client, 
        int bufsize=kReceptionBufSize
    );
    HttpConnection(
        ClientConnection client, 
        std::shared_ptr<logrr::Logger> logger, 
        int bufsize=kReceptionBufSize
    );
    HttpConnection(
        ClientConnection client, 
        std::shared_ptr<logrr::StatusLogger> slogger, 
        int bufsize=kReceptionBufSize
    );
    virtual ~HttpConnection();
    virtual void process(const Router& router);
protected:
    virtual bool recv() noexcept;
    virtual void send(const std::string&) noexcept;
    std::string execution(const Router& router) noexcept;
    void closeConnection(int& sockfd) noexcept;
protected:
    ClientConnection client_; 
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
    std::string req_;
    int bufsize_;
};

} // namespace http

#endif