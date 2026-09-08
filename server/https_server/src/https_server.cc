#include "https_server.h"

namespace https {

HttpsServer::HttpsServer(
    const std::string& cert, 
    const std::string& key, 
    http::IRouter& router
) : http::HttpServer(router)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_ERROR("Failed to create SSL_CTX object", {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
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

        LOG_ERROR("Failed to load certificate file", {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
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

        LOG_ERROR("Failed to load private key file", {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
        });
        throw SSLException(buf);
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
        std::string msg = "Private key does not match certificate";
        LOG_ERROR(msg);
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
            LOG_WARN("A secure connection with the client was not established");
            continue;
        }

        HttpsConnection connection(ssl, client, bufsize);
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

        LOG_ERROR("A secure connection with the client was not established", {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
        });
        return nullptr;
    }

    SSL_set_fd(ssl, client.sockfd);

    if (SSL_accept(ssl) <= 0) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_WARN("Failed to perform SSL handshake", {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
        });
        SSL_free(ssl);
        return nullptr;
    }
    else {
        LOG_INFO("TLS handshake successful");
    }

    return ssl;
}

} // namespace https