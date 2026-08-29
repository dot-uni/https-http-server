#include "https_server.h"

namespace https {

HttpsServer::HttpsServer(
    const std::string& cert, 
    const std::string& key, 
    http::IRouter& router,
    std::shared_ptr<logrr::Logger> logger
) : http::HttpServer(router, logger)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_ERROR(this->logger_, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
            logrr::field("message", "Failed to create SSL_CTX object")
        });
        throw SSLException(buf);
    }

    if (SSL_CTX_use_certificate_chain_file(
        ctx_,
        cert.c_str()) <= 0) 
    {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_ERROR(this->logger_, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
            logrr::field("message", "Failed to load certificate file")
        });
        throw SSLException(buf);
    }

    if (SSL_CTX_use_PrivateKey_file(
        ctx_,
        key.c_str(),
        SSL_FILETYPE_PEM) <= 0)
    {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_ERROR(this->logger_, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
            logrr::field("message", "Failed to load private key file")
        });
        throw SSLException(buf);
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
        std::string msg = "Private key does not match certificate";
        LOG_ERROR(this->logger_, {
            logrr::field("message", msg)
        });
        throw SSLException(msg);
    }
}


HttpsServer::~HttpsServer() 
{
    SSL_CTX_free(ctx_);
    EVP_cleanup();
}


void HttpsServer::clientIntakeCycle(int bufsize) noexcept 
{
    http::ClientConnection client;
    while(true) {
        client = this->acceptConnection();
        if (client.sockfd == http::kInvalidSocket) continue;
        
        SSL* ssl = sslHandshake(client);
        if (!ssl) {
            LOG_WARN(this->logger_, {
                logrr::field("message", "A secure connection with the client was not established")
            });
            continue;
        }

        HttpsConnection connection(ssl, client, this->logger_, bufsize);
        connection.process(this->router_);
    }
}


SSL* HttpsServer::sslHandshake(const http::ClientConnection& client) noexcept 
{
    SSL* ssl = SSL_new(ctx_);
    if (!ssl) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_ERROR(this->logger_, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
            logrr::field("message", "Failed to create SSL object")
        });
        return nullptr;
    }

    SSL_set_fd(ssl, client.sockfd);

    if (SSL_accept(ssl) <= 0) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_WARN(this->logger_, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
            logrr::field("message", "Failed to perform SSL handshake")
        });
        SSL_free(ssl);
        return nullptr;
    }
    else {
        LOG_INFO(this->logger_, {
            logrr::field("message", "TLS handshake successful")
        });
    }

    return ssl;
}

} // namespace https