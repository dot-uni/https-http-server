#include "https_server.h"

namespace https {

HttpsServer::HttpsServer(std::shared_ptr<logrr::ILogSink> logsink) : HttpServer(logsink)
{
    if (logger_) logger_->lCalled(__func__);

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        if (logger_) logger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
        });
        throw SSLException(buf);
    }

    if (SSL_CTX_use_certificate_file(
        ctx_,
        "cert.pem",
        SSL_FILETYPE_PEM) <= 0) 
    {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        if (logger_) logger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
        });
        throw SSLException(buf);
    }

    if (SSL_CTX_use_PrivateKey_file(
        ctx_,
        "key.pem",
        SSL_FILETYPE_PEM) <= 0)
    {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        if (logger_) logger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
        });
        throw SSLException(buf);
    }

    if (!SSL_CTX_check_private_key(ctx_)) {
        std::string msg = "Private key does not match certificate";
        if (logger_) logger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("message", msg)
        });
        throw SSLException(msg);
    }

    if (logger_) logger_->lExeced(__func__, {
        logrr::field("message", "HttpsServer object created")
    });
}


HttpsServer::~HttpsServer() 
{
    if (logger_) logger_->lCalled(__func__);

    SSL_CTX_free(ctx_);
    EVP_cleanup();

    if (logger_) logger_->lExeced(__func__, {
        logrr::field("message", "HttpsServer has shut down")
    });
}


void HttpsServer::clientIntakeCycle(int bufsize) noexcept 
{
    if (logger_) logger_->lCalled(__func__);

    http::ClientConnection client;
    while(true) {
        client = acceptConnection();
        if (client.sockfd == http::kInvalidSocket) continue;
        
        SSL* ssl = sslHandshake(client);
        if (!ssl) {
            if (logger_) logger_->lWarning(__func__, __FILE__, __LINE__, {
                logrr::field("message", "A secure connection with the client was not established")
            });
            continue;
        }

        HttpsConnection connection(ssl, client, logger_, bufsize);
        connection.process();
    }

    if (logger_) logger_->lExeced(__func__);
}


SSL* HttpsServer::sslHandshake(const http::ClientConnection& client) noexcept 
{
    if (logger_) logger_->lCalled(__func__);

    SSL* ssl = SSL_new(ctx_);
    if (!ssl) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        if (logger_) logger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
        });
        return nullptr;
    }

    SSL_set_fd(ssl, client.sockfd);

    if (SSL_accept(ssl) <= 0) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));

        if (logger_) logger_->lError(__func__, __FILE__, __LINE__, {
            logrr::field("OpenSSL_error_code", ssl_err),
            logrr::field("OpenSSL_error_string", buf),
        });
        return nullptr;
    }
    else {
        if (logger_) logger_->lInfo(__func__, {
            logrr::field("message", "TLS handshake successful")
        });
    }

    return ssl;

    if (logger_) logger_->lExeced(__func__);
}


} // namespace https