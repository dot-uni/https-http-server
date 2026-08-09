#ifndef HTTPS_CONNECTION_INCLUDED
#define HTTPS_CONNECTION_INCLUDED

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_connection.h"

namespace https {

class HttpsConnection : public http::HttpConnection
{
public:
    HttpsConnection(
        SSL* ssl,
        const http::ClientConnection& client, 
        int bufsize=http::kReceptionBufSize
    );
    HttpsConnection(
        SSL* ssl,
        const http::ClientConnection& client, 
        std::shared_ptr<logrr::Logger> logger, 
        int bufsize=http::kReceptionBufSize
    );
    HttpsConnection(
        SSL* ssl,
        const http::ClientConnection& client, 
        std::shared_ptr<logrr::StatusLogger> slogger, 
        int bufsize=http::kReceptionBufSize
    );
    virtual ~HttpsConnection();
    void process() override;
protected:
    bool recv() noexcept override;
    void send(const std::string&) noexcept override;
protected:
    SSL* ssl_;
};
} // namespace https 

#endif