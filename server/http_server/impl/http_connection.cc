#include "http_connection.h"

namespace http {
    
HttpConnection::HttpConnection(
    ClientConnection client, 
    int bufsize
) : client_(std::move(client)), bufsize_(bufsize) {}


HttpConnection::HttpConnection(
    ClientConnection client, 
    std::shared_ptr<logrr::Logger>& logger, 
    int bufsize
) : client_(std::move(client)), slogger_(std::static_pointer_cast<logrr::StatusLogger>(logger)), bufsize_(bufsize) {}


HttpConnection::HttpConnection(
    ClientConnection client, 
    std::shared_ptr<logrr::StatusLogger>& slogger, 
    int bufsize
) : client_(std::move(client)), slogger_(slogger), bufsize_(bufsize) {}


HttpConnection::~HttpConnection() 
{
    closeConnection(client_.sockfd);
    LOG_INFO(slogger_, {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port),
        logrr::field("message", "Client socket was closed")
    });
}


bool HttpConnection::process(const IRouter& router) 
{
    if (!HttpConnection::recv()) return false;
    std::string resp = execution(router);
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
            LOG_ERROR(slogger_, {
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno)),
                logrr::field("message", "Error from ::recv")
            });
            Response resp = makeResp(retCode::InternalError);
            HttpConnection::send(HttpCodec::serialize(resp));
            return false; 
        }
        else if (numbytes == 0) {
            LOG_WARN(slogger_, {
                logrr::field("message", "Client disconnected")
            });
            return false;
        }

        resbytes += numbytes;
        if (resbytes >= kReceptionBufLimit) {
            LOG_USING_RETCODE(retCode::RequestBufferOverflow, slogger_);
            Response resp = makeResp(retCode::RequestBufferOverflow);
            HttpConnection::send(HttpCodec::serialize(resp));
            return false;
        }

        buf[numbytes] = '\0';
        req.append(buf);

        LOG_INFO(slogger_, {
            logrr::field("client_id", client_.id),
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port),
            logrr::field("message", frmt::concat(numbytes, " bytes were received"))
        });

        if (numbytes < bufsize_) break;
    } 

    req_ = std::move(req);
    
    LOG_INFO(slogger_, {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port),
        logrr::field("message", frmt::concat("A total of ", resbytes, " bytes received from the client"))   
    });
    return true;
}


std::string HttpConnection::execution(const IRouter& router) noexcept {
    HttpCodec codec(slogger_);
    std::string resp = codec.process(req_, router); 
    return resp;
}


bool HttpConnection::send(const std::string& resp) noexcept 
{
    int numbytes = 0;
    int all_bytes = resp.size();
    std::string r = resp;
    while(true) {
        numbytes = ::send(client_.sockfd, r.c_str(), all_bytes, 0);
        if (numbytes >= all_bytes) break;
        all_bytes -= numbytes;
        r = r.substr(numbytes);
    }
    return true;
}


void HttpConnection::closeConnection(int& sockfd) noexcept 
{
    if (sockfd > 0) {
        close(sockfd);
        sockfd = kEmptyDescriptor;
    }
    else sockfd = kInvalidSocket;
}

} // namespace http


