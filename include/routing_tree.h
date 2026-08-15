#ifndef ROUTING_TREE_INCLUDED
#define ROUTING_TREE_INCLUDED

#include <vector>
#include <unordered_map>
#include <optional>

#include "http_message.h"


namespace http {

using Handler = std::function<Response(Request&&)>;

std::vector<std::string> parse(std::string path);

class RoutingTree 
{
    struct RNode 
    {
        std::string id;
        std::vector<std::shared_ptr<RNode>> childs; 
        std::optional<Handler> h = std::nullopt;
        RNode(
            std::string id, 
            std::optional<Handler> h = std::nullopt
        );
    };

    using Childs = std::unordered_map<std::string, std::vector<std::shared_ptr<RNode>>>;
    Childs root_;   
    size_t size_;
public:
    RoutingTree() = default;
    virtual ~RoutingTree() = default;

    std::optional<Handler> get(const Method& mtd, const std::string& path) const noexcept;
    bool add(const Method& mtd, const std::string& path, Handler& h) noexcept;
    size_t size() const noexcept;
    bool empty() const noexcept;
private:
    bool add(const Method& mtd, std::vector<std::string>&& path_elems, Handler h) noexcept;
};

} // namespace http

#endif