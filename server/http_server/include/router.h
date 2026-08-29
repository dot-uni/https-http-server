#ifndef ROUTER_INCLUDED
#define ROUTER_INCLUDED

#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>

#include "routing_tree.h"
#include "status_logging.h"
#include "ret_status.h" 
#include "crypto.h"


namespace http {

class IRouter
{
public:
    virtual ~IRouter() = default;
    virtual Response match(Request&& req) const noexcept = 0;
};


template <
    typename Hasher = cryp::SipHasher,
    typename HashKey = cryp::SipHashKey,
    typename Hash = cryp::Hash<Hasher, HashKey>
> class Router final : public IRouter {
public:
    Router() = default;
    Router(const HashKey& key);
    ~Router() = default;

    bool get(std::string_view path, Handler&& h) noexcept;
    bool post(std::string_view path, Handler&& h) noexcept;
    bool put(std::string_view path, Handler&& h) noexcept;
    bool del(std::string_view path, Handler&& h) noexcept;
    bool patch(std::string_view path, Handler&& h) noexcept;

    Response match(Request&& req) const noexcept override;
private:
    RoutingTree<Hasher, HashKey, Hash> rtree_;
};


template <typename Hasher, typename HashKey, typename Hash>
Router<Hasher, HashKey, Hash>::Router(const HashKey& key) : rtree_(key) {}


template <typename Hasher, typename HashKey, typename Hash>
bool Router<Hasher, HashKey, Hash>::get(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(Method::GET, path, std::move(h));
}


template <typename Hasher, typename HashKey, typename Hash>
bool Router<Hasher, HashKey, Hash>::post(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(Method::POST, path, std::move(h));
}


template <typename Hasher, typename HashKey, typename Hash>
bool Router<Hasher, HashKey, Hash>::put(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(Method::PUT, path, std::move(h));
}


template <typename Hasher, typename HashKey, typename Hash>
bool Router<Hasher, HashKey, Hash>::del(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(Method::DELETE, path, std::move(h));
}


template <typename Hasher, typename HashKey, typename Hash>
bool Router<Hasher, HashKey, Hash>::patch(std::string_view path, Handler&& h) noexcept
{
    return rtree_.add(Method::PATCH, path, std::move(h));
}


template <typename Hasher, typename HashKey, typename Hash>
Response Router<Hasher, HashKey, Hash>::match(Request&& req) const noexcept 
{
    Handler h = rtree_.get(req.method, req.path);
    if (!h) {
        return makeResp(retCode::NotFound, req.id);
    }
    Response resp = h(std::move(req));
    return resp;
}

} // namespace http

#endif