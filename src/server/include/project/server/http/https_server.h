#ifndef HTTPS_SERVER_INCLUDED
#define HTTPS_SERVER_INCLUDED

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "project/server/http/http_server.h"
#include "project/server/http/https_connection.h"



namespace uni::server::http {

class SSLException : public std::runtime_error 
{
public:
    explicit SSLException(const std::string& msg) : std::runtime_error(msg) {}
};


class HttpsServer final : public HttpServer
{
public:
    HttpsServer (
        const std::string& cert, 
        const std::string& key
    );
    ~HttpsServer();

    HttpsServer(const HttpsServer&) = delete;
    HttpsServer& operator=(const HttpsServer&) = delete;

    HttpsServer(HttpsServer&& serv) = delete;
    HttpsServer& operator=(HttpsServer&& serv) = delete;
private:
    void clientIntakeCycle(int bufsize) noexcept override;
    SSL* sslHandshake(const ClientConnection&) noexcept;
private:
    SSL_CTX* ctx_;
};

} // uni::server::http

#endif