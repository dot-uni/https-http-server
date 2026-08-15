#include "http_connection.h"

namespace http {


HttpConnection::HttpConnection(
    ClientConnection client, 
    int bufsize
) : client_(std::move(client)), bufsize_(bufsize) {}


HttpConnection::HttpConnection(
    ClientConnection client, 
    std::shared_ptr<logrr::Logger> logger, 
    int bufsize
) : client_(std::move(client)), slogger_(std::static_pointer_cast<logrr::StatusLogger>(logger)), bufsize_(bufsize) {}


HttpConnection::HttpConnection(
    ClientConnection client, 
    std::shared_ptr<logrr::StatusLogger> slogger, 
    int bufsize
) : client_(std::move(client)), slogger_(std::move(slogger)), bufsize_(bufsize) {}


HttpConnection::~HttpConnection() 
{
    closeConnection(client_.sockfd);
    if (slogger_) slogger_->lInfo(__FILE_NAME__, __LINE__, __func__, {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port),
        logrr::field("message", "Client socket was closed") 
    });
}

void HttpConnection::process(const Router& router) 
{
    if (!HttpConnection::recv()) return;
    std::string resp = execution(router);
    HttpConnection::send(resp);
}


bool HttpConnection::recv() noexcept 
{
    int numbytes, resbytes = 0;
    char buf[bufsize_];
    std::string req;

    while(true) {
        numbytes = ::recv(client_.sockfd, buf, sizeof(buf), 0);
        if (numbytes == -1) {
            if (slogger_) slogger_->lError(__FILE_NAME__, __LINE__, __func__, {
                logrr::field("errno", errno),
                logrr::field("strerror", strerror(errno)),
                logrr::field("message", "Error from ::recv")
            });
            Response resp = makeResp(retCode::InternalError);
            HttpConnection::send(HttpCodec::serialize(resp));
            return false; 
        }
        else if (numbytes == 0) {
            if (slogger_) slogger_->lWarning(__FILE_NAME__, __LINE__, __func__, {
                logrr::field("message", "Client disconnected"),
            });
            return false;
        }

        resbytes += numbytes;
        if (resbytes >= kReceptionBufLimit) {
            if (slogger_) slogger_->log(retCode::RequestBufferOverflow, __FILE_NAME__, __LINE__, __func__);
            Response resp = makeResp(retCode::RequestBufferOverflow);
            HttpConnection::send(HttpCodec::serialize(resp));
            return false;
        }

        buf[numbytes] = '\0';
        req.append(buf);

        if (slogger_) slogger_->lInfo(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("client_id", client_.id),
            logrr::field("client_ip", client_.ip),
            logrr::field("client_port", client_.port),
            logrr::field("message", tostr::concat(numbytes, " bytes were received"))
        });

        if (numbytes < bufsize_) break;
    } 

    req_ = std::move(req);
    
    if (slogger_) slogger_->lInfo(__FILE_NAME__, __LINE__, __func__, {
        logrr::field("client_id", client_.id),
        logrr::field("client_ip", client_.ip),
        logrr::field("client_port", client_.port),
        logrr::field("message", tostr::concat("A total of ", resbytes, " bytes received from the client"))
    });
    return true;
}


std::string HttpConnection::execution(const Router& router) noexcept {
    HttpCodec codec(slogger_);
    std::string resp = codec.process(req_, router); 
    return resp;
}

void HttpConnection::send(const std::string& resp) noexcept 
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