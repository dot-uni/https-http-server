#include "project/routing/router.h"


namespace uni::routing {

namespace status = ::uni::common::status;
using ::uni::logging::field;


void RouterManager::Clear() noexcept
{
    LOG_DEBUG("Resetting router instance");
    router_ = nullptr;
}


bool RouterManager::Get(std::string_view path, Handler h) 
{
    if (!router_) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = router_->get(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RouterManager::Post(std::string_view path, Handler h) 
{
    if (!router_) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = router_->post(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RouterManager::Put(std::string_view path, Handler h) 
{
    if (!router_) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = router_->put(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RouterManager::Del(std::string_view path, Handler h) 
{
    if (!router_) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = router_->del(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


bool RouterManager::Patch(std::string_view path, Handler h) 
{
    if (!router_) {
        LOG_ERROR("Router not initialized", {
            field("path", path)
        });
        throw std::runtime_error("Router not initialized");
    }

    LOG_TRACE("Registering handler", {
        field("path", path)
    });
    bool result = router_->patch(path, std::move(h));
    if (!result) {
        LOG_WARN("Failed to register handler", {
            field("path", path)
        });
    }

    return result;
}


std::optional<server::http::Response> RouterManager::Route(const server::http::Request& req) noexcept
{
    if (!router_) {
        LOG_ERROR("Router not initialized, request dropped");
        return std::nullopt;
    }

    LOG_TRACE("Routing request");
    auto response = router_->route(req);

    if (!response) {
        LOG_DEBUG("No matching route found");
        return server::http::makeResp(status::retCode::NotFound, req.id);
    }
    return *response;
}

} // namespace uni::routing