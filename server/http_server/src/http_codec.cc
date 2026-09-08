#include "http_codec.h"

namespace http {

std::optional<Request> HttpCodec::parse(std::string_view raw_req) 
{
    LOG_TRACE("Parsing incoming request", {
        logrr::field("raw_size", raw_req.size())
    });

    Request req;
    int end_targets = raw_req.find("\r\n");
    int end_headers = raw_req.find("\r\n\r\n");
    if (end_targets == std::string::npos) {
        LOG_WARN("The request is not in the correct format");
        return std::nullopt;
    }

    /// parse target
    std::string_view targets = raw_req.substr(0, end_targets);

    int first_space = targets.find(' ');
    int second_space = targets.find(' ', first_space+1);
    req.method = to_method(targets.substr(0, first_space));
    if (req.method == Method::UNKNOWN) {
        LOG_WARN("Incorrectly specified method <UNKNOWN>", {
            logrr::field("raw_target", std::string(targets))
        });
        return std::nullopt;
    }
    req.path = targets.substr(first_space+1, second_space-first_space-1);
    req.version = targets.substr(second_space+1);

    LOG_TRACE("Target line parsed", {
        logrr::field("method", to_string(req.method)),
        logrr::field("path", std::string(req.path))
    });

    /// parse header
    std::string_view headers = raw_req.substr(end_targets+2,  end_headers);

    int beg = 0;
    int end = headers.find("\r\n"), colon;
    if (end == std::string::npos) {
        LOG_WARN("The header field is missing from the request");
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

        std::string key(headers.substr(beg, colon - beg));
        std::string value(headers.substr(colon + 1, end - colon - 1));

        size_t val_start = value.find_first_not_of(" \t");
        if (val_start != std::string::npos) value = value.substr(val_start);

        req.headers[key] = value;

        beg = end + 2;
        end = headers.find("\r\n", beg);
        if (end == std::string::npos) break;
    }
    if (!req.headers.count("Host")) {
        LOG_WARN("Host is not specified in header");
        return std::nullopt;
    }

    LOG_TRACE("Headers parsed", {
        logrr::field("header_count", req.headers.size())
    });

    /// parse body
    if (end_headers != std::string::npos) {
        std::string_view body = raw_req.substr(end_headers+4);
        try {
            if (!body.empty()) {
                req.body = nlohmann::json::parse(body); 
            }
        } catch(nlohmann::json::parse_error& mess) {
            LOG_WARN("Failed to parse JSON body", {
                logrr::field("error", mess.what())
            });
            return std::nullopt;
        }
    }

    req.id = uuid::generate_uuid_v4();
    LOG_DEBUG("Request successfully parsed", {
        logrr::field("request_id", req.id)
    });
    return req;
}


std::string HttpCodec::serialize(Response& resp) noexcept 
{
    LOG_TRACE("Serializing response", {
        logrr::field("status", frmt::to_string(resp.status))
    });

    std::string body = resp.body.dump(4);
    std::string out = fmt::format("HTTP/1.1 {} {}\r\nContent-Type: application/json\r\nContent-Length: {}\r\n", 
        frmt::to_string(resp.status), obsolete_reason(resp.status), body.size());

    std::for_each(resp.headers.begin(), resp.headers.end(), [&out](const auto& kv){
        out += fmt::format("{}: {}\r\n", kv.first, kv.second);
    });

    out += "\r\n";
    out += body;
    
    return out;
}


bool HttpCodec::parse_w(std::string_view raw_req) 
{
    auto req = HttpCodec::parse(raw_req);
    if (!req) return false;
    req_ = *req;
    return true;
}


std::string HttpCodec::process(std::string_view raw_req, const IRouter& router) {
    Response resp;
    if (!parse_w(raw_req)) {
        LOG_DEBUG("Request parsing failed, returning error response");
        resp = makeResp(retCode::InvalidJsonOrParams);
    }
    else {
        resp = router.match(std::move(req_)); 
    }
    LOG_USING_STATUS(resp.status, {
        logrr::field("method", to_string(req_.method)),
        logrr::field("path", req_.path)
    });
    return HttpCodec::serialize(resp);
}

} // namespace http