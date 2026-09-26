#ifndef HTTPS_CONNECTION_INCLUDED
#define HTTPS_CONNECTION_INCLUDED

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "project/server/http/http_connection.h"


namespace uni::server::http {

class HttpsConnection : public HttpConnection
{
public:
    HttpsConnection(
        SSL* ssl,
        ClientConnection&& client, 
        int bufsize=kReceptionBufSize
    );
    virtual ~HttpsConnection();
    bool process() override;
protected:
    bool recv() noexcept override;
    bool send(const std::string&) noexcept override;
protected:
    SSL* ssl_;
};

} // namespace uni::server::http

#endif