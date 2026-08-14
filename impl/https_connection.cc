#include "https_connection.h"

namespace https {

HttpsConnection::HttpsConnection(
    SSL* ssl,
    const http::ClientConnection& client, 
    int bufsize
) : HttpConnection(client, bufsize), ssl_(ssl) {}


HttpsConnection::HttpsConnection(
    SSL* ssl,
    const http::ClientConnection& client, 
    std::shared_ptr<logrr::Logger> logger, 
    int bufsize
) : HttpConnection(client, logger, bufsize), ssl_(ssl) {}


HttpsConnection::HttpsConnection(
    SSL* ssl,
    const http::ClientConnection& client, 
    std::shared_ptr<logrr::StatusLogger> slogger, 
    int bufsize
) : HttpConnection(client, slogger, bufsize), ssl_(ssl) {}


HttpsConnection::~HttpsConnection()
{
    if (slogger_) slogger_->lCalled(__func__);

    SSL_shutdown(ssl_);
    SSL_free(ssl_);

    if (slogger_) slogger_->lExeced(__func__);
}


void HttpsConnection::process(const http::Router& router) 
{
    if (slogger_) slogger_->lCalled(__func__);
    if (!HttpsConnection::recv()) return;
    std::string resp = execution(router);
    HttpsConnection::send(resp);
}


bool HttpsConnection::recv() noexcept 
{
    if (slogger_) slogger_->lCalled(__func__);

    char buf[bufsize_];
    std::string req;
    int resbytes = 0;

    while (true) {
        int numbytes = SSL_read(ssl_, buf, sizeof(buf));

        if (numbytes > 0) {
            resbytes += numbytes;
            if (resbytes >= http::kReceptionBufLimit) {
                if (slogger_) slogger_->log(http::retCode::RequestBufferOverflow, __func__, __LINE__);
                http::Response resp = makeResp(http::retCode::RequestBufferOverflow);
                HttpsConnection::send(http::HttpCodec::serialize(resp));
                return false;
            }

            req.append(buf, numbytes);

            if (slogger_) slogger_->lInfo(__func__, {
                logrr::field("client_id", client_.id),
                logrr::field("client_ip", client_.ip),
                logrr::field("client_port", client_.port),
                logrr::field("message", tostr::concat(numbytes, " bytes were received"))
            });

            if (numbytes < bufsize_) break; 
            continue;                        
        }

        int err = SSL_get_error(ssl_, numbytes);

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }

        if (err == SSL_ERROR_ZERO_RETURN) {
            if (slogger_) slogger_->lWarning(__func__, __FILE__, __LINE__, {
                logrr::field("message", "Client closed TLS connection (close_notify)"),
            });
            return false;
        }

        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));

        if (slogger_) slogger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("message", "Error from SSL_read"),
            logrr::field("ssl_error_code", err),
            logrr::field("errno", errno),
            logrr::field("errno_str", strerror(errno)),
            logrr::field("openssl_error", errbuf)
        });

        http::Response resp = makeResp(http::retCode::InternalError);
        HttpsConnection::send(http::HttpCodec::serialize(resp));
        return false;
    }

    req_ = std::move(req);

    if (slogger_) slogger_->lExeced(__func__, {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port),
        logrr::field("message", tostr::concat("A total of ", resbytes, " bytes received from the client"))
    });
    return true;
}


void HttpsConnection::send(const std::string& resp) noexcept 
{
    if (slogger_) slogger_->lCalled(__func__);

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
            continue;
        }

        if (err == SSL_ERROR_ZERO_RETURN) {
            if (slogger_) slogger_->lWarning(__func__, __FILE__, __LINE__, {
                logrr::field("message", "Connection closed by peer during SSL_write"),
            });
            return;
        }

        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));

        if (slogger_) slogger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("message", "Error from SSL_write"),
            logrr::field("ssl_error_code", err),
            logrr::field("errno", errno),
            logrr::field("errno_str", strerror(errno)),
            logrr::field("openssl_error", errbuf)
        });
        return;
    }

    if (slogger_) slogger_->lExeced(__func__);
}

} // namespace https 