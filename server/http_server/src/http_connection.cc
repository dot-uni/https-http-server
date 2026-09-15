#include "http_connection.h"

namespace http {
    
HttpConnection::HttpConnection(
    ClientConnection client, 
    int bufsize
) : client_(std::move(client)), bufsize_(bufsize) 
{
    LOG_DEBUG("New HttpConnection created", {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port),
        logrr::field("bufsize", bufsize_)
    });
}


HttpConnection::~HttpConnection() 
{
    closeConnection(client_.sockfd);
    LOG_DEBUG("Client socket was closed", {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port)
    });
}


bool HttpConnection::process() 
{
    LOG_TRACE("Processing new request", {
        logrr::field("client_id", client_.id)
    });

    if (!HttpConnection::recv()) return false;
    std::string resp = execution();
    return HttpConnection::send(resp);
}


bool HttpConnection::recv() noexcept 
{
    int numbytes, resbytes = 0;
    char buf[bufsize_];
    std::string req;

    while(true) {
        numbytes = ::recv(client_.sockfd, buf, sizeof(buf), 0);
        if (numbytes == -1) {
            LOG_ERROR("Error from ::recv", {
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno))
            });
            Response resp = makeResp(retCode::InternalError);
            HttpConnection::send(HttpCodec::serialize(resp));
            return false; 
        }
        else if (numbytes == 0) {
            LOG_DEBUG("Client disconnected");
            return false;
        }

        resbytes += numbytes;
        if (resbytes >= kReceptionBufLimit) {
            LOG_USING_RETCODE(retCode::RequestBufferOverflow);
            Response resp = makeResp(retCode::RequestBufferOverflow);
            HttpConnection::send(HttpCodec::serialize(resp));
            return false;
        }

        buf[numbytes] = '\0';
        req.append(buf);

        LOG_TRACE("`{}` bytes were received", {
            logrr::field("client_id", client_.id),
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port)
        }) << numbytes;
        if (numbytes < bufsize_) break;
    } 

    req_ = std::move(req);
    
    LOG_DEBUG("A total of `{}` bytes received from the client", {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port)
    }) << resbytes;
    return true;
}


std::string HttpConnection::execution() noexcept 
{
    LOG_TRACE("Routing request", {
        logrr::field("client_id", client_.id)
    });

    HttpCodec codec;
    std::string resp = codec.process(req_); 
    return resp;

    LOG_TRACE("Response generated", {
        logrr::field("client_id", client_.id),
        logrr::field("resp_size", resp.size())
    });
}


bool HttpConnection::send(const std::string& resp) noexcept 
{
    size_t total_send = 0;
    const size_t total_size = resp.size();
    
    while (total_send < total_size) {
        ssize_t numbytes = ::send(client_.sockfd, resp.c_str() + total_send, total_size - total_send, 0);

        if (numbytes < 0) {
            if (errno == EINTR) continue; 
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;

            LOG_ERROR("Error from ::send", {
                logrr::field("client_id", client_.id),
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno))
            });
            return false;
        }
        if (numbytes == 0) {
            LOG_WARN("Send returned 0, connection likely closed", {
                logrr::field("client_id", client_.id)
            });
            return false;
        }
        total_send += static_cast<size_t>(numbytes);
    }

    LOG_TRACE("Response sent successfully", {
        logrr::field("client_id", client_.id),
        logrr::field("bytes_sent", total_size)
    });
    return true;
}


void HttpConnection::closeConnection(int& sockfd) noexcept 
{
    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else {
        LOG_WARN("Attempted to close an already invalid socket", {
            logrr::field("sockfd", sockfd)
        });
        sockfd = kInvalidSocket;
    }
}

} // namespace http


