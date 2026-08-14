#include "http_message.h"

namespace http {

std::string toString(Method m) 
{
    switch(static_cast<Method>(m)) {
        case Method::GET:           return "GET";
        case Method::POST:          return "POST";
        case Method::PUT:           return "PUT";
        case Method::DELETE:        return "DELETE";
        case Method::PATCH:         return "PATCH";
        default:
            break;
    }
    return "UNKNOWN";
}

Method toMethod(const std::string& s)
{   
    if (s.length() > 6) return Method::UNKNOWN;

    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), 
                    [](unsigned char c) { return std::tolower(c); });

    if (lower == "get")         return Method::GET;
    if (lower == "post")        return Method::POST;
    if (lower == "put")         return Method::PUT;
    if (lower == "delete")      return Method::DELETE;
    if (lower == "patch")       return Method::PATCH;
    return Method::UNKNOWN;
}

int64_t get_timestamp_ms() 
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

Response makeResp(
    http::retCode retcode, 
    const std::string& id, 
    const nlohmann::ordered_json& result) 
{
    return Response{
        .status = toHttpStatus(retcode),
        .headers {
            {"X-Request-Id", id}
        },
        .body = {
            {"retCode", retcode},
            {"retMesg", retMesg(retcode)},
            {"time", get_timestamp_ms()},
            {"result", result}
        }
    };
}

Response makeResp(
    http::retCode retcode, 
    const nlohmann::ordered_json& result) 
{ 
    return makeResp(retcode, uuid::generate_uuid_v4(), result); 
}

} // namespace http