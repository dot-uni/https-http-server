#include "project/server/http/http_server.h"


namespace uni::server::http {

using ::uni::logging::field;

HttpServer::HttpServer()
{
    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = AF_UNSPEC;
    hints_.ai_socktype = SOCK_STREAM;
    hints_.ai_protocol = IPPROTO_TCP;
    hints_.ai_flags = AI_PASSIVE;

    LOG_DEBUG("HttpServer instance created");
}


HttpServer::~HttpServer() 
{   
    freeAddrInfo(servinfo_);
    closeConnection(sockfd_);
    LOG_DEBUG("HttpServer instance destroyed");
}


void HttpServer::swap(HttpServer& other) noexcept
{
    std::swap(sockfd_, other.sockfd_);
    std::swap(servinfo_, other.servinfo_);
    std::swap(hints_, other.hints_);
    std::swap(is_running_, other.is_running_);
}


bool HttpServer::listen(const char* host, const char* port, int max_connections, int bufsize) 
{
    LOG_INFO("Starting HTTP server", {
        field("host", host),
        field("port", port),
        field("max_connections", max_connections)
    });

    bool ok = buildSocket(host, port) && listenInternal(max_connections, bufsize);

    if (!ok) {
        LOG_CRIT("HttpServer failed to start listening — server cannot accept connections", {
            field("host", host),
            field("port", port)
        });
    }
    return ok;
}


bool HttpServer::buildSocket(const char* host, const char* port) noexcept 
{
    int success, sockfd, opt = 1;
    addrinfo *servinfo, *p, *next;

    success = getaddrinfo(host, port, &hints_, &servinfo);
    if (success != 0) {
        LOG_ERROR("Error from ::getaddrinfo()", {
            field("gai_strerror", gai_strerror(success))
        });
        return false;
    }

    for (p = servinfo; p != nullptr;) {
        next = p->ai_next;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == kInvalidSocket) {
            LOG_WARN("Error from ::socket(), trying next address", {
                field("errno", errno),
                field("strerror", strerror(errno))
            });
            freeaddrinfo(p);
            p = next;
            continue;
        }

        success = setSockOptions(sockfd, SO_REUSEADDR, SO_REUSEPORT);
        if (!success) {
            LOG_ERROR("Error from HttpServer::setSockOptions()", {
                field("errno", errno),
                field("strerror", strerror(errno))
            });
            freeaddrinfo(p);
            closeConnection(sockfd);
            return false;
        }

        success = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (success == -1) {
            LOG_WARN("Error from ::bind(), trying next address", {
                field("errno", errno),
                field("strerror", strerror(errno))
            });
            closeConnection(sockfd);
            p = next;
            continue;
        }
        break;
    }

    if (!p) {
        LOG_CRIT("HttpServer failed to bind to any resolved address", {
            field("host", host),
            field("port", port)
        });
        return false;
    }

    LOG_DEBUG("Socket bound successfully", {
        field("sockfd", sockfd)
    });

    closeConnection(sockfd_);
    servinfo_ = std::move(p);
    sockfd_ = std::move(sockfd);
    return true;
}


void HttpServer::freeAddrInfo(addrinfo*& servinfo) noexcept 
{
    LOG_TRACE("Freeing addrinfo");
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
    int success = setsockopt(sockfd, SOL_SOCKET, arg, &opt, sizeof(opt));
    if (success == -1) {
        LOG_ERROR("Error from ::setsockopt()", {
            field("errno", errno),
            field("strerror", strerror(errno))
        });
        return false;
    }
    return true;
}


bool HttpServer::listenInternal(int max_connections, int bufsize) noexcept
{
    int success;
    if (max_connections <= 0) {
        LOG_ERROR("The number of connections must be greater than 0", {
            field("max_connections", max_connections)
        });
        return false;
    }

    if (bufsize <= 0) {
        LOG_ERROR("The buffer size must be strictly greater than 0", {
            field("bufsize", bufsize)
        });
        return false;
    }

    success = ::listen(sockfd_, max_connections);
    if (success == -1) {
        LOG_CRIT("Error from ::listen() — server cannot accept connections", {
            field("errno", errno),
            field("strerror", strerror(errno))
        });
        return false;
    }

    LOG_INFO("Server listening for connections", {
        field("max_connections", max_connections),
        field("bufsize", bufsize)
    });
    clientIntakeCycle(bufsize);
    return true;
}


void HttpServer::clientIntakeCycle(int bufsize) noexcept 
{
    pid_t pid;
    while(true) {
        ClientConnection client = acceptConnection();
        if (client.sockfd == kInvalidSocket) {
            LOG_WARN("Skipping invalid client connection");
            continue;
        }
       
        LOG_TRACE("Dispatching new HttpConnection", {
            field("client_id", client.id)
        });
        

        if ((pid = fork()) == 0) {
            close(sockfd_);
            {
                HttpConnection connection(std::move(client), bufsize);
                connection.process();
            }
            _exit(0);
        }
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
        LOG_ERROR("Error from ::accept()", {
            field("errno", errno),
            field("strerror", strerror(errno))
        });
        return ClientConnection();
    }
    
    cli_id = uni::common::uuid::generate_uuid_v4();
    cli_ip = getIpAddr((sockaddr*)&cli_addr);
    cli_port = ntohs(getSinPort((sockaddr*)&cli_addr));

    LOG_INFO("Accepted new client connection", {
        field("client_id", cli_id),
        field("client_ip", cli_ip),
        field("client_port", cli_port)
    });
    return ClientConnection{cli_id, cli_sock, cli_ip, cli_port};
}

} // namespace uni::server::http