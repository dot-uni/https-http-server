#ifndef HTTP_MESSAGE_INCLUDED
#define HTTP_MESSAGE_INCLUDED

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <functional>
#include <cctype>
#include <string>
#include <chrono>

#include "status.h"
#include "ret_status.h"

namespace http {


struct CaseInsensitiveHash {
    size_t operator()(const std::string& s) const {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        return std::hash<std::string>{}(lower);
    }
};

struct CaseInsensitiveEqual {
    bool operator()(const std::string& a, const std::string& b) const {
        if (a.size() != b.size()) return false;
        return std::equal(a.begin(), a.end(), b.begin(),
            [](unsigned char c1, unsigned char c2) {
                return std::tolower(c1) == std::tolower(c2);
            });
    }
};


enum class Method : uint8_t {
    GET=0,
    POST,
    PUT,
    DELETE,
    PATCH,
    UNKNOWN
};

std::string toString(Method m);
Method toMethod(const std::string& s);

struct Request 
{
    Method method;
    std::string path="";
    std::string version="HTTP/1.1";
    std::unordered_map<std::string, std::string, 
                       CaseInsensitiveHash, CaseInsensitiveEqual> headers;
    nlohmann::ordered_json body;
};


struct Response 
{
    http::status status;
    std::unordered_map<std::string, std::string, 
                       CaseInsensitiveHash, CaseInsensitiveEqual> headers;
    nlohmann::ordered_json body;
};

int64_t get_timestamp_ms();

Response makeResp(
    http::retCode retcode, 
    const std::string& id, 
    const nlohmann::ordered_json& result=nlohmann::json::object()
);

} // namespace http

#endif