#include "http_server.h"


namespace {
    void* getSinAddr(sockaddr *sa) 
    {
        if (sa->sa_family == AF_INET) {
            return &(((sockaddr_in*)sa)->sin_addr);
        }
        else if (sa->sa_family == AF_INET6) {
            return &(((sockaddr_in6*)sa)->sin6_addr);
        }
        return nullptr;
    }

    uint16_t getSinPort(sockaddr *sa) 
    {
        if (sa->sa_family == AF_INET) {
            return (((sockaddr_in*)sa)->sin_port);
        }
        else if (sa->sa_family == AF_INET6) {
            return (((sockaddr_in6*)sa)->sin6_port);
        }
        return 0;
    }

    std::string getIpAddr(sockaddr* addr) 
    {
        char str_addr[INET6_ADDRSTRLEN];
        const char* success;

        success = inet_ntop(addr->sa_family, getSinAddr(addr), str_addr, sizeof(str_addr));
        if (success == nullptr) {
            return "";
        }
        return std::string(str_addr);
    }
} // namespace


namespace http {

    
HttpServer::HttpServer(const Router& router, std::shared_ptr<logrr::ILogSink> logsink) : router_(router)
{
    if (logsink) {
        logger_ = std::make_shared<logrr::Logger>();
        if (logger_ && !logger_->addSink(logsink)) {
            logger_.reset();
            logger_ = nullptr;
        }
    }

    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = AF_UNSPEC;
    hints_.ai_socktype = SOCK_STREAM;
    hints_.ai_protocol = IPPROTO_TCP;
    hints_.ai_flags = AI_PASSIVE;
}


HttpServer::~HttpServer() 
{   
    freeAddrInfo(servinfo_);
    closeConnection(sockfd_);
}

void HttpServer::moveImpl(HttpServer&& serv) noexcept 
{
    sockfd_ = std::move(serv.sockfd_);
    servinfo_ = std::move(serv.servinfo_);
    hints_ = std::move(serv.hints_);
    logger_ = std::move(serv.logger_);
    is_running_ = std::move(serv.is_running_);
}

HttpServer::HttpServer(HttpServer&& serv) noexcept 
{
    moveImpl(std::move(serv));
}

HttpServer& HttpServer::operator=(HttpServer&& serv) noexcept 
{
    if (&serv == this) return *this;
    freeAddrInfo(servinfo_);
    closeConnection(sockfd_);
    moveImpl(std::move(serv));
    return *this;
}

bool HttpServer::listen(const char* host, const char* port, int max_connections, int bufsize) 
{
    if (logger_) logger_->lInfo(__FILE_NAME__, __LINE__, __func__, {
        logrr::field("host", host)
    });
    return buildSocket(host, port) && listenInternal(max_connections, bufsize);
}

bool HttpServer::buildSocket(const char* host, const char* port) noexcept 
{
    int success, sockfd, opt = 1;
    addrinfo *servinfo, *p, *next;

    success = getaddrinfo(host, port, &hints_, &servinfo);
    if (success != 0) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("gai_strerror", gai_strerror(success)),
            logrr::field("message", "Error from ::getaddrinfo()")
        });
        return false;
    }

    for (p = servinfo; p != nullptr;) {
        next = p->ai_next;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == kInvalidSocket) {
            if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno)),
                logrr::field("message", "Error from ::socket()")
            });
            freeaddrinfo(p);
            p = next;
            continue;
        }

        success = setSockOptions(sockfd, SO_REUSEADDR, SO_REUSEPORT);
        if (success == -1) {
            if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno)),
                logrr::field("message", "Error from http::HttpServer::setSockOptions()")
            });
            freeaddrinfo(p);
            closeConnection(sockfd);
            return false;
        }

        success = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (success == -1) {
            if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno)),
                logrr::field("message", "Error from ::bind()")
            });
            freeaddrinfo(p);
            closeConnection(sockfd);
            p = next;
            continue;
        }
        break;
    }

    if (!p) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "HttpServer failed to bind")
        });
        return false;
    }

    closeConnection(sockfd_);
    servinfo_ = std::move(p);
    sockfd_ = std::move(sockfd);
    return true;
}

void HttpServer::freeAddrInfo(addrinfo*& servinfo) noexcept 
{
    freeaddrinfo(servinfo);
    servinfo = nullptr;
}

void HttpServer::closeConnection(int& sockfd) noexcept 
{
    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else sockfd = kInvalidSocket;
}

template <typename... Opts>
bool HttpServer::setSockOptions(int sockfd, Opts&&... args) noexcept 
{
    int opt = 1;
    bool success = (applyOption<Opts>(sockfd, std::forward<Opts>(args), opt), ...);
    return success;
}

template <typename Opt> bool HttpServer::applyOption(int sockfd, Opt&& arg, int opt) noexcept 
{
    int success;
    success = setsockopt(sockfd, SOL_SOCKET, arg, &opt, sizeof(opt));
    if (success == -1) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("message", "Error from ::setsockopt()")
        });
        return false;
    }
    return true;
}

bool HttpServer::listenInternal(int max_connections, int bufsize) noexcept
{
    int success;
    if (max_connections <= 0) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "The number of connections must be greater than 0")
        });
        return false;
    }

    if (bufsize <= 0) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "The buffer size must be strictly greater than 0")
        });
        return false;
    }

    success = ::listen(sockfd_, max_connections);
    if (success == -1) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("message", "Error from ::listen()")
        });
        return false;
    }

    if (logger_) logger_->lInfo(__FILE_NAME__, __LINE__, __func__, {
        logrr::field("message", "Server waiting for connections...")
    });

    clientIntakeCycle(bufsize);
    return true;
}

void HttpServer::clientIntakeCycle(int bufsize) noexcept 
{
    ClientConnection client;
    while(true) {
        client = acceptConnection();
        if (client.sockfd == kInvalidSocket) continue;
        
        HttpConnection connection(client, logger_, bufsize);
        connection.process(router_);
    }
}

ClientConnection HttpServer::acceptConnection() noexcept
{
    int cli_sock;
    socklen_t cli_size;
    sockaddr_storage cli_addr;
    std::string cli_ip, cli_id;
    uint16_t cli_port;
    
    cli_size = sizeof(cli_addr);
    cli_sock = accept(sockfd_, (sockaddr*)&cli_addr, &cli_size);

    if (cli_sock == kInvalidSocket) {
        if (logger_) logger_->lError(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("errno", errno),
            logrr::field("strerror", strerror(errno)),
            logrr::field("message", "Error from ::accept()")
        });
        return ClientConnection();
    }
    
    cli_id = uuid::generate_uuid_v4();
    cli_ip = getIpAddr((sockaddr*)&cli_addr);
    cli_port = ntohs(getSinPort((sockaddr*)&cli_addr));

    if (logger_) logger_->lInfo(__FILE_NAME__, __LINE__, __func__, {
        logrr::field("client_id", cli_id),
        logrr::field("client_ip", cli_ip),
        logrr::field("client_port", cli_port),
        logrr::field("message", "Accepted new client connection")
    });

    return ClientConnection{cli_id, cli_sock, cli_ip, cli_port};
}

bool HttpServer::addSink(std::shared_ptr<logrr::ILogSink> sink) noexcept 
{
    if (logger_ && logger_->addSink(sink)) {
        return true;
    }
    return false;
}

} // namespace http