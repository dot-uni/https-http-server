#include "router.h"

namespace http {

Router::Router(std::shared_ptr<logrr::StatusLogger> slogger) : slogger_(slogger) {}


std::unordered_map<std::string, Route>& Router::registry() 
{
    static std::unordered_map<std::string, Route> instance;
    return instance;
}

Router::AutoRegistry::AutoRegistry(const std::string& method, const std::string& path, Handler h, bool auth_req) 
{
    Router::registry().emplace(method+path, std::move(Route{
        .handler = h,
        .auth_req = auth_req
    }));
}

bool Router::add(const std::string& method, const std::string& path, Handler h, bool auth_req)
{
    std::string key = method + path;
    if (Router::registry().count(key)) return false;

    Router::registry().emplace(key, std::move(Route{
        .handler = h,
        .auth_req = auth_req
    }));
    return true;
}



Response Router::route(Request&& req, const std::string& id) const noexcept 
{
    if (slogger_) slogger_->lCalled(__func__);

    std::string key = req.method + req.path;
    if (!Router::registry().count(key)) {
        return makeResp(retCode::NotFound, id);
    }
    Route& r = Router::registry()[key];

    Response resp;
    if (r.auth_req) {
        /**
         * Authentication verification is required here
         * 
         *      X-Timestamp: xxxx
         *      X-Recv-Window: xxx
         *      X-Signature: xxxxxx
         */
        resp = r.handler(std::forward<Request>(req), id);
    }
    else {
        resp = r.handler(std::forward<Request>(req), id);
    }
    if (slogger_) slogger_->lExeced(__func__);
    return resp;
}
} // namespace http