#ifndef ROUTER_INCLUDED
#define ROUTER_INCLUDED

#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>

#include "project/routing/routing_tree.h"
#include "project/common/status/retcode.h" 
#include "project/routing/crypto.h"
#include "project/logging/manager/logging_system.h"


namespace uni::routing {

class RouterBase
{
public:
    virtual ~RouterBase() = default;

    virtual bool get(std::string_view path, Handler&& h) noexcept = 0;
    virtual bool post(std::string_view path, Handler&& h) noexcept = 0;
    virtual bool put(std::string_view path, Handler&& h) noexcept = 0;
    virtual bool del(std::string_view path, Handler&& h) noexcept = 0;
    virtual bool patch(std::string_view path, Handler&& h) noexcept = 0;

    virtual std::optional<server::http::Response> route(server::http::Request req) const noexcept = 0;
};


template <
    typename HashKey = cryp::SipHashKey,
    typename Hash = cryp::SipHash
> class Router final : public RouterBase {
public:
    Router() = default;
    Router(const HashKey& key) : rtree_(key) {}
    ~Router() = default;

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    Router(Router&&) noexcept = default;
    Router& operator=(Router&&) noexcept = default;

    bool get(std::string_view path, Handler&& h) noexcept override;
    bool post(std::string_view path, Handler&& h) noexcept override;
    bool put(std::string_view path, Handler&& h) noexcept override;
    bool del(std::string_view path, Handler&& h) noexcept override;
    bool patch(std::string_view path, Handler&& h) noexcept override;

    std::optional<server::http::Response> route(server::http::Request req) const noexcept override;
private:
    RoutingTree<HashKey, Hash> rtree_;
};


template <typename HashKey, typename Hash>
bool Router<HashKey, Hash>::get(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(server::http::Method::GET, path, std::move(h));
}


template <typename HashKey, typename Hash>
bool Router<HashKey, Hash>::post(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(server::http::Method::POST, path, std::move(h));
}


template <typename HashKey, typename Hash>
bool Router<HashKey, Hash>::put(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(server::http::Method::PUT, path, std::move(h));
}


template <typename HashKey, typename Hash>
bool Router<HashKey, Hash>::del(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(server::http::Method::DELETE, path, std::move(h));
}


template <typename HashKey, typename Hash>
bool Router<HashKey, Hash>::patch(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(server::http::Method::PATCH, path, std::move(h));
}


template <typename HashKey, typename Hash>
std::optional<server::http::Response> Router<HashKey, Hash>::route(server::http::Request req) const noexcept 
{
    Handler h = rtree_.get(req.method, req.path);
    if (!h) {
        return std::nullopt;
    }
    server::http::Response resp = h(std::move(req));
    return resp;
}


class RouterManager final
{
public:
    template <
        typename HashKey = cryp::SipHashKey,
        typename Hash = cryp::SipHash
    > static void Init();

    template <
        typename HashKey = cryp::SipHashKey,
        typename Hash = cryp::SipHash
    > static void Init(const HashKey& key);

    static void Clear() noexcept;

    static bool Get(std::string_view path, Handler h);
    static bool Post(std::string_view path, Handler h);
    static bool Put(std::string_view path, Handler h);
    static bool Del(std::string_view path, Handler h);
    static bool Patch(std::string_view path, Handler h);
    
    static std::optional<server::http::Response> Route(const server::http::Request& req) noexcept;
private:
    inline static std::unique_ptr<RouterBase> router_ = nullptr;
};


template <typename HashKey, typename Hash> 
void RouterManager::Init()
{
    if (router_) {
        LOG_WARN("Router already initialized, replacing existing instance");
    }
    LOG_DEBUG("Initializing router with default hash key");
    router_ = std::make_unique<Router<HashKey, Hash>>();
}


template <typename HashKey, typename Hash> 
void RouterManager::Init(const HashKey& key)
{
    if (router_) {
        LOG_WARN("Router already initialized, replacing existing instance");
    }
    LOG_DEBUG("Initializing router with provided hash key");
    router_ = std::make_unique<Router<HashKey, Hash>>(key);
}

} // namespace uni::routing

#endif