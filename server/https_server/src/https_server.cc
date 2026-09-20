#include "https_server.h"

namespace https {

HttpsServer::HttpsServer(
    const std::string& cert, 
    const std::string& key
) 
{
    LOG_DEBUG("Initializing TLS context");

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_CRIT("Failed to create SSL_CTX object — server cannot start in HTTPS mode", {
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

        LOG_CRIT("Failed to load certificate file — server cannot start in HTTPS mode", {
            logrr::field("cert_path", cert),
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

        LOG_CRIT("Failed to load private key file — server cannot start in HTTPS mode", {
            logrr::field("key_path", key),
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
        });
        throw SSLException(buf);
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
        std::string msg = "Private key does not match certificate — server cannot start in HTTPS mode";
        LOG_CRIT(msg);
        throw SSLException(msg);
    }

    LOG_INFO("TLS context initialized successfully", {
        logrr::field("cert_path", cert)
    });
}


HttpsServer::~HttpsServer() 
{
    SSL_CTX_free(ctx_);
    EVP_cleanup();
    LOG_DEBUG("HttpsServer destroyed, SSL_CTX freed");
}


void HttpsServer::clientIntakeCycle(int bufsize) noexcept 
{
    pid_t pid;
    while(true) {
        http::ClientConnection client = this->acceptConnection();
        if (client.sockfd == http::kInvalidSocket) {
            LOG_WARN("Skipping invalid client connection");
            continue;
        }
        
        SSL* ssl = sslHandshake(client);
        if (!ssl) {
            LOG_DEBUG("Secure connection with the client was not established");
            continue;
        }

        LOG_TRACE("Dispatching new HttpsConnection", {
            logrr::field("client_id", client.id)
        });

        HttpsConnection connection(ssl, std::move(client), bufsize);
        connection.process();
    }
}


SSL* HttpsServer::sslHandshake(const http::ClientConnection& client) noexcept 
{
    SSL* ssl = SSL_new(ctx_);
    if (!ssl) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        LOG_ERROR("Failed to allocate SSL object", {
            logrr::field("client_id", client.id),
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

        LOG_DEBUG("TLS handshake failed", { 
            logrr::field("client_id", client.id),
            logrr::field("client_ip", client.ip),
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf)
        });
        SSL_free(ssl);
        return nullptr;
    }
    
    LOG_DEBUG("TLS handshake successful", { // было INFO -> DEBUG
        logrr::field("client_id", client.id),
        logrr::field("client_ip", client.ip)
    });

    return ssl;
}

} // namespace https