#ifndef ROUTER_INCLUDED
#define ROUTER_INCLUDED

#include <string>
#include <unordered_map>
#include <functional>

#include "status_logging.h"
#include "http_message.h"
#include "ret_status.h" 

namespace http {

using Handler = std::function<Response(Request&&, const std::string&)>;

struct Route 
{
    Handler handler;
    bool auth_req = true;
};

class Router {
public:
    Router() = default;
    Router(std::shared_ptr<logrr::StatusLogger>);
    virtual ~Router() = default;

    static std::unordered_map<std::string, Route>& registry();
    struct AutoRegistry 
    {
        AutoRegistry(const std::string& method, const std::string& path, Handler h, bool auth_req=true);
    };
    static bool add(const std::string& method, const std::string& path, Handler h, bool auth_req=true);

    Response route(Request&& req, const std::string& id) const noexcept;
protected:
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
};

#define HTTP_CONCAT_IMPL(a, b) a##b
#define HTTP_CONCAT(a, b) HTTP_CONCAT_IMPL(a, b)

#define HTTP_REGISTER_HANDLER(method, path, func, auth) \
    static ::http::Router::AutoRegistry HTTP_CONCAT(_auto_reg_, __LINE__)(method, path, func, auth)

} // namespace http

#endif