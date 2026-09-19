#include "https_connection.h"

namespace https {

HttpsConnection::HttpsConnection(
    SSL* ssl,
    http::ClientConnection&& client, 
    int bufsize
) : http::HttpConnection(std::move(client), bufsize), ssl_(ssl) 
{
    LOG_DEBUG("New HttpsConnection created", {
        logrr::field("client_id", client.id),
        logrr::field("client_ip", client.ip),
        logrr::field("client_port", client.port)
    });
}


HttpsConnection::~HttpsConnection()
{
    int result = SSL_shutdown(ssl_);
    if (result < 0) {
        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));
        LOG_WARN("SSL_shutdown did not complete cleanly", {
            logrr::field("client_id", this->client_.id),
            logrr::field("openssl_error", errbuf)
        });
    }
    SSL_free(ssl_);
    LOG_DEBUG("HttpsConnection destroyed, SSL freed", {
        logrr::field("client_id", this->client_.id)
    });
}


bool HttpsConnection::process() 
{
    LOG_TRACE("Processing new HTTPS request", {
        logrr::field("client_id", this->client_.id)
    });

    if (!HttpsConnection::recv()) return false;
    std::string resp = this->execution();
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
                LOG_USING_RETCODE(http::retCode::RequestBufferOverflow);
                http::Response resp = makeResp(http::retCode::RequestBufferOverflow);
                HttpsConnection::send(http::HttpCodec::serialize(resp));
                return false;
            }

            req.append(buf, numbytes);

            LOG_TRACE("`{}` bytes were received", {
                logrr::field("client_id", this->client_.id),
                logrr::field("client_ip", this->client_.ip),
                logrr::field("client_port", this->client_.port)
            }) << numbytes;
            if (numbytes < this->bufsize_) break; 
            continue;                        
        }

        int err = SSL_get_error(ssl_, numbytes);

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            LOG_TRACE("SSL_read returned WANT_READ/WANT_WRITE, retrying");
            continue;
        }

        if (err == SSL_ERROR_ZERO_RETURN) {
            LOG_DEBUG("Client closed TLS connection (close_notify)");
            return false;
        }

        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));

        LOG_ERROR("Error from SSL_read", {
            logrr::field("client_id", this->client_.id),
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("ssl_error_code", err),
            logrr::field("openssl_error", errbuf)
        });

        http::Response resp = makeResp(http::retCode::InternalError);
        HttpsConnection::send(http::HttpCodec::serialize(resp));
        return false;
    }

    this->req_ = std::move(req);

    LOG_DEBUG("A total of `{}` bytes received from the client", { 
        logrr::field("client_id", this->client_.id),
        logrr::field("client_ip", this->client_.ip),
        logrr::field("client_port", this->client_.port)
    }) << resbytes;
    return true;
}


 
bool HttpsConnection::send(const std::string& resp) noexcept 
{
    size_t total_sent = 0;
    size_t total_size = resp.size();

    while (total_sent < total_size) {
        int chunk = static_cast<int>(total_size - total_sent);
        int numbytes = SSL_write(ssl_, resp.c_str() + total_sent, chunk);

        if (numbytes > 0) {
            total_sent += static_cast<size_t>(numbytes);
            continue;
        }

        int err = SSL_get_error(ssl_, numbytes);

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            LOG_TRACE("SSL_write returned WANT_READ/WANT_WRITE, retrying");
            continue;
        }

        if (err == SSL_ERROR_ZERO_RETURN) {
            LOG_DEBUG("Connection closed by peer during SSL_write"); 
            return false;
        }

        unsigned long sslErr = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(sslErr, errbuf, sizeof(errbuf));

        LOG_ERROR("Error from SSL_write", {
            logrr::field("client_id", this->client_.id),
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("ssl_error_code", err),
            logrr::field("openssl_error", errbuf)
        });
        return false;
    }

    LOG_TRACE("Response sent successfully", {
        logrr::field("client_id", this->client_.id),
        logrr::field("bytes_sent", total_size)
    });
    return true;
}

} // namespace https