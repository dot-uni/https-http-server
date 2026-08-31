#include "http_codec.h"

namespace http {

HttpCodec::HttpCodec(std::shared_ptr<logrr::Logger>& logger) 
    : slogger_(std::static_pointer_cast<logrr::StatusLogger>(logger)) {}


HttpCodec::HttpCodec(std::shared_ptr<logrr::StatusLogger>& slogger) 
    : slogger_(slogger) {}


std::optional<Request> HttpCodec::parse(const std::string& raw_req) 
{
    Request req;
    int end_targets = raw_req.find("\r\n");
    int end_headers = raw_req.find("\r\n\r\n");
    if (end_targets == std::string::npos) {
        LOG_WARN(slogger_, {{"message", "The request is not in the correct format"}});
        return std::nullopt;
    }

    /// parse target
    std::string targets = raw_req.substr(0, end_targets);

    int first_space = targets.find(' ');
    int second_space = targets.find(' ', first_space+1);
    req.method = to_method(targets.substr(0, first_space));
    if (req.method == Method::UNKNOWN) {
        LOG_WARN(slogger_, {{"message", "Incorrectly specified method <UNKNOWN>"}});
        return std::nullopt;
    }
    req.path = targets.substr(first_space+1, second_space-first_space-1);
    req.version = targets.substr(second_space+1);

    /// parse header
    std::string headers = raw_req.substr(end_targets+2,  end_headers);

    int beg = 0;
    int end = headers.find("\r\n"), colon;
    if (end == std::string::npos) {
        LOG_WARN(slogger_, {{"message", "The header field is missing from the request"}});
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
        LOG_WARN(slogger_, {{"message", "Host is not specified in header"}});
        return std::nullopt;
    }

    /// parse body
    if (end_headers != std::string::npos) {
        std::string body = raw_req.substr(end_headers+4);
        try {
            if (!body.empty()) {
                req.body = nlohmann::json::parse(body); 
            }
        } catch(nlohmann::json::parse_error& mess) {
            LOG_WARN(slogger_, {{"message", mess.what()}});
            return std::nullopt;
        }
    }

    req.id = uuid::generate_uuid_v4();
    return req;
}


std::string HttpCodec::serialize(Response& resp) noexcept 
{
    std::string out = "";
    std::string body = resp.body.dump(4);

    out += "HTTP/1.1 " + frmt::to_string(resp.status) + " " + std::string(obsolete_reason(resp.status)) + "\r\n";

    out += "Content-Type: application/json\r\n";
    out += "Content-Length: " + frmt::to_string(body.size()) + "\r\n";
    std::for_each(resp.headers.begin(), resp.headers.end(), [&out](const auto& kv){
        out += kv.first + ": " + kv.second + "\r\n";
    });
    out += "\r\n";

    out += body;
    return out;
}


bool HttpCodec::parse_w(const std::string& raw_req) 
{
    auto req = HttpCodec::parse(raw_req);
    if (!req) return false;
    req_ = *req;
    return true;
}


std::string HttpCodec::process(const std::string& raw_req, const IRouter& router) {
    Response resp;
    if (!parse_w(raw_req)) {
        resp = makeResp(retCode::InvalidJsonOrParams);
    }
    else {
        resp = router.match(std::move(req_)); 
    }
    LOG_USING_HTTP_STS(resp.status, slogger_, {
        logrr::field("method", to_string(req_.method)),
        logrr::field("path", req_.path)
    });
    return HttpCodec::serialize(resp);
}

} // namespace http