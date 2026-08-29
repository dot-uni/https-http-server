#include "https_connection.h"

namespace https {

HttpsConnection::HttpsConnection(
    SSL* ssl,
    const http::ClientConnection& client, 
    int bufsize
) : http::HttpConnection(client, bufsize), ssl_(ssl) {}



HttpsConnection::HttpsConnection(
    SSL* ssl,
    const http::ClientConnection& client, 
    std::shared_ptr<logrr::Logger>& logger, 
    int bufsize
) : http::HttpConnection(client, logger, bufsize), ssl_(ssl) {}



HttpsConnection::HttpsConnection(
    SSL* ssl,
    const http::ClientConnection& client, 
    std::shared_ptr<logrr::StatusLogger>& slogger, 
    int bufsize
) : http::HttpConnection(client, slogger, bufsize), ssl_(ssl) {}



HttpsConnection::~HttpsConnection()
{
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
}



bool HttpsConnection::process(const http::IRouter& router) 
{
    if (!HttpsConnection::recv()) return false;
    std::string resp = this->execution(router);
    return HttpsConnection::send(resp);
}



bool HttpsConnection::recv() noexcept 
{
    char buf[this->bufsize_];
    std::string req;
    int resbytes = 0;

    while (true) {
        int numbytes = SSL_read(ssl_, buf, sizeof(buf));

        if (numbytes > 0) {
            resbytes += numbytes;
            if (resbytes >= http::kReceptionBufLimit) {
                LOG_USING_RETCODE(http::retCode::RequestBufferOverflow, this->slogger_);
                http::Response resp = makeResp(http::retCode::RequestBufferOverflow);
                HttpsConnection::send(http::HttpCodec::serialize(resp));
                return false;
            }

            req.append(buf, numbytes);

            LOG_INFO(this->slogger_, {
                logrr::field("client_id", this->client_.id),
                logrr::field("client_ip", this->client_.ip),
                logrr::field("client_port", this->client_.port),
                logrr::field("message", frmt::concat(numbytes, " bytes were received"))
            });

            if (numbytes < this->bufsize_) break; 
            continue;                        
        }

        int err = SSL_get_error(ssl_, numbytes);

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            LOG_INFO(this->slogger_, {
                logrr::field("message", "SSL_read returned SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE, continuing to read")
            });
            continue;
        }

        if (err == SSL_ERROR_ZERO_RETURN) {
            LOG_WARN(this->slogger_, {
                logrr::field("message", "Client closed TLS connection (close_notify)")
            });
            return false;
        }

        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));

        LOG_ERROR(this->slogger_, {
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("ssl_error_code", err),
            logrr::field("openssl_error", errbuf),
            logrr::field("message", "Error from SSL_read")
        });

        http::Response resp = makeResp(http::retCode::InternalError);
        HttpsConnection::send(http::HttpCodec::serialize(resp));
        return false;
    }

    this->req_ = std::move(req);

    LOG_INFO(this->slogger_, {
        logrr::field("client_id", this->client_.id),
        logrr::field("client_ip", this->client_.ip),
        logrr::field("client_port", this->client_.port),
        logrr::field("message", frmt::concat("A total of ", resbytes, " bytes received from the client"))
    });
    return true;
}


 
bool HttpsConnection::send(const std::string& resp) noexcept 
{
    size_t total_sent = 0;
    size_t all_bytes = resp.size();

    while (total_sent < all_bytes) {
        int chunk = static_cast<int>(all_bytes - total_sent);
        int numbytes = SSL_write(ssl_, resp.data() + total_sent, chunk);

        if (numbytes > 0) {
            total_sent += static_cast<size_t>(numbytes);
            continue;
        }

        int err = SSL_get_error(ssl_, numbytes);

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            LOG_INFO(this->slogger_, {
                logrr::field("message", "SSL_write returned SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE, continuing to write")
            });
            continue;
        }

        if (err == SSL_ERROR_ZERO_RETURN) {
            LOG_WARN(this->slogger_, {
                logrr::field("message", "Connection closed by peer during SSL_write")
            });
            return false;
        }

        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));

        LOG_ERROR(this->slogger_, {
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("ssl_error_code", err),
            logrr::field("openssl_error", errbuf),
            logrr::field("message", "Error from SSL_write")
        });
        return false;
    }
    return true;
}

} // namespace https