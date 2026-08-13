#ifndef ROUTER_INCLUDED
#define ROUTER_INCLUDED

#include <string>
#include <unordered_map>
#include <functional>

#include "routing_tree.h"
#include "status_logging.h"
#include "ret_status.h" 

namespace http {

class Router {
public:
    Router() = default;
    Router(std::shared_ptr<logrr::StatusLogger>);
    virtual ~Router() = default;
public:
    bool get(const std::string& path, Handler h) noexcept;
    bool post(const std::string& path, Handler h) noexcept;
    bool put(const std::string& path, Handler h) noexcept;
    bool del(const std::string& path, Handler h) noexcept;
    bool patch(const std::string& path, Handler h) noexcept;

    Response route(Request&& req, const std::string& id) const noexcept;
protected:
    RoutingTree rtree_;
    std::shared_ptr<logrr::StatusLogger> slogger_ = nullptr;
};

} // namespace http

#endif