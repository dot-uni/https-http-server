#ifndef HTTPS_CONNECTION_INCLUDED
#define HTTPS_CONNECTION_INCLUDED

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "project/server/http_connection.h"


namespace uni {
namespace https {

class HttpsConnection : public http::HttpConnection
{
public:
    HttpsConnection(
        SSL* ssl,
        http::ClientConnection&& client, 
        int bufsize=http::kReceptionBufSize
    );
    virtual ~HttpsConnection();
    bool process() override;
protected:
    bool recv() noexcept override;
    bool send(const std::string&) noexcept override;
protected:
    SSL* ssl_;
};

} // namespace https 
} // namespace uni

#endif