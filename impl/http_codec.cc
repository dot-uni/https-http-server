#include "http_codec.h"


namespace http {


HttpCodec::HttpCodec(std::shared_ptr<logrr::Logger> logger) 
    : slogger_(std::static_pointer_cast<logrr::StatusLogger>(logger)) {}

Request HttpCodec::parse(const std::string& raw_req) 
{
    Request req;
    int end_targets = raw_req.find("\r\n");
    int end_headers = raw_req.find("\r\n\r\n");

    if (end_targets == std::string::npos || end_headers == std::string::npos) {
        throw std::invalid_argument("Bad request"); 
    }

    std::string targets = raw_req.substr(0, end_targets);
    std::string headers = raw_req.substr(end_targets+2,  end_headers);
    std::string body = raw_req.substr(end_headers+4);

    /// parse target
    int first_space = targets.find(' ');
    int second_space = targets.find(' ', first_space+1);
    req.method = targets.substr(0, first_space);
    req.path = targets.substr(first_space, second_space);
    req.version = targets.substr(second_space);


    /// parse header
    int beg = 0;
    int end = headers.find("\r\n"), colon;
    if (end == std::string::npos) {
        throw std::invalid_argument("The header field is missing from the request");
    }

    while(true) {
        colon = headers.find(":", beg);
        if (colon == std::string::npos || colon > end) {
            beg = end + 2;
            end = headers.find("\r\n", beg);
            if (end == std::string::npos) break;
            continue;
        }

        std::string key = headers.substr(beg, colon - beg);
        std::string value = headers.substr(colon + 1, end - colon - 1);

        size_t val_start = value.find_first_not_of(" \t");
        if (val_start != std::string::npos) value = value.substr(val_start);

        req.headers[key] = value;

        beg = end + 2;
        end = headers.find("\r\n", beg);
        if (end == std::string::npos) break;
    }
    if (!req.headers.count("Host")) {
        throw std::invalid_argument("Host is not specified in header");
    }

    /// parse body
    try {
        if (!body.empty()) {
            req.body = nlohmann::json::parse(body); 
        }
    } catch(nlohmann::json::parse_error& mess) {
        throw std::invalid_argument("The provided body is not in JSON format");
    }

    return req;
}


std::string HttpCodec::serialize(Response& resp) noexcept 
{
    std::string targets =  "HTTP/1.1 " + tostr::convertToString(resp.status) + " " + std::string(obsolete_reason(resp.status)) + "\r\n";
    std::string headers = "";
    std::string body = resp.body.dump(4);

    headers += "Content-Type: application/json\r\n";
    headers += "Content-Length: " + tostr::convertToString(body.size()) + "\r\n";
    for (auto&& [key, value] : resp.headers) {
        headers += key + ": " + value + "\r\n";
    }
    headers += "\r\n";

    return targets + headers + body;
}

bool HttpCodec::parse_w(const std::string& raw_req) 
{
    if (slogger_) slogger_->lCalled(__func__);

    try {
        req_ = HttpCodec::parse(raw_req);
    } catch(std::invalid_argument& mess) {
        if (slogger_) slogger_->log(retCode::InvalidJsonOrParams, __func__, __LINE__, {
            logrr::field("message", mess.what())
        });
        return false;
    }

    if (slogger_) slogger_->lExeced(__func__);
    return true;
}

std::string HttpCodec::process(const std::string& raw_req, const std::string& id) {
    if (slogger_) slogger_->lCalled(__func__);

    Response resp;
    if (!parse_w(raw_req)) {
        resp = makeResp(retCode::InvalidJsonOrParams, id);
    }
    else {
        Router router(slogger_);
        resp = router.route(std::forward<Request>(req_), id); 
    }

    if (slogger_) slogger_->lExeced(__func__);
    return HttpCodec::serialize(resp);
}

} // namespace http