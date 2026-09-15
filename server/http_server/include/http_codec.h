#ifndef HTTP_CODEC_INCLUDED
#define HTTP_CODEC_INCLUDED

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <optional>

#include "uuid.h"
#include "logging.h"
#include "http_message.h"
#include "tostring.h"
#include "router.h"

namespace http {


class HttpCodec
{
public:
    HttpCodec() = default;
    virtual ~HttpCodec() = default;

    std::string process(std::string_view raw_req);
    std::optional<Request> parse(std::string_view raw_req);
    static std::string serialize(Response& resp) noexcept;
protected:
    bool parse_w(std::string_view raw_req);
protected:
    Request req_;
};

} // namespace http

#endif 