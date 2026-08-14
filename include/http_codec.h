#ifndef HTTP_CODEC_INCLUDED
#define HTTP_CODEC_INCLUDED

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>

#include "uuid.h"
#include "status_logging.h"
#include "status.h"
#include "http_message.h"
#include "tostring.h"
#include "router.h"

namespace http {


class HttpCodec
{
public:
    HttpCodec() = default;
    HttpCodec(std::shared_ptr<logrr::Logger>);
    virtual ~HttpCodec() = default;

    std::string process(const std::string& raw_req, const Router& router);
    static Request parse(const std::string& raw_req);
    static std::string serialize(Response& resp) noexcept;
protected:
    bool parse_w(const std::string& raw_req);
protected:
    Request req_;
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
};


} // namespace http

#endif 