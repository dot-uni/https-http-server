#ifndef HTTPS_SERVER_INCLUDED
#define HTTPS_SERVER_INCLUDED

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_server.h"
#include "https_connection.h"

namespace https {

class SSLException : public std::runtime_error 
{
public:
    explicit SSLException(const std::string& msg) : std::runtime_error(msg) {}
};


class HttpsServer : public http::HttpServer 
{
public:
    HttpsServer() : HttpsServer(std::make_shared<logrr::ConsoleSink>()) {}
    HttpsServer(std::shared_ptr<logrr::ILogSink>);
    virtual ~HttpsServer();
protected:
    void clientIntakeCycle(int bufsize) noexcept override;
    SSL* sslHandshake(const http::ClientConnection&) noexcept;
protected:
    SSL_CTX* ctx_;
};

} // namespace https

#endif