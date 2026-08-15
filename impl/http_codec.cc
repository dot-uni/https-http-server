#include "http_codec.h"


namespace http {


HttpCodec::HttpCodec(std::shared_ptr<logrr::Logger> logger) 
    : slogger_(std::static_pointer_cast<logrr::StatusLogger>(logger)) {}

std::optional<Request> HttpCodec::parse(const std::string& raw_req) 
{
    Request req;
    int end_targets = raw_req.find("\r\n");
    int end_headers = raw_req.find("\r\n\r\n");

    if (end_targets == std::string::npos || end_headers == std::string::npos) {
        if (slogger_) slogger_->lWarning(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "The request is not in the correct format")
        });
        return std::nullopt;
    }

    std::string targets = raw_req.substr(0, end_targets);
    std::string headers = raw_req.substr(end_targets+2,  end_headers);
    std::string body = raw_req.substr(end_headers+4);

    /// parse target
    int first_space = targets.find(' ');
    int second_space = targets.find(' ', first_space+1);
    req.method = toMethod(targets.substr(0, first_space));
    if (req.method == Method::UNKNOWN) {
        if (slogger_) slogger_->lWarning(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "Incorrectly specified method <UNKNOWN>")
        });
        return std::nullopt;
    }
    req.path = targets.substr(first_space+1, second_space-first_space-1);
    req.version = targets.substr(second_space+1);

    /// parse header
    int beg = 0;
    int end = headers.find("\r\n"), colon;
    if (end == std::string::npos) {
        if (slogger_) slogger_->lWarning(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "The header field is missing from the request")
        });
        return std::nullopt;
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
        if (slogger_) slogger_->lWarning(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "Host is not specified in header")
        });
        return std::nullopt;
    }

    /// parse body
    try {
        if (!body.empty()) {
            req.body = nlohmann::json::parse(body); 
        }
    } catch(nlohmann::json::parse_error& mess) {
        if (slogger_) slogger_->lWarning(__FILE_NAME__, __LINE__, __func__, {
            logrr::field("message", "The provided body is not in JSON format")
        });
        return std::nullopt;
    }

    req.id = uuid::generate_uuid_v4();
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
    auto req = HttpCodec::parse(raw_req);
    if (!req) return false;
    req_ = *req;
    return true;
}

std::string HttpCodec::process(const std::string& raw_req, const Router& router) {
    Response resp;
    if (!parse_w(raw_req)) {
        resp = makeResp(retCode::InvalidJsonOrParams);
    }
    else {
        resp = router.route(std::move(req_)); 
    }
    return HttpCodec::serialize(resp);
}

} // namespace http