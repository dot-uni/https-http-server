#ifndef ROUTING_TREE_INCLUDED
#define ROUTING_TREE_INCLUDED

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <optional>

#include "http_message.h"
#include "crypto.h"
#include "net_constants.h"


namespace http {

using Handler = std::function<Response(Request&&)>;


template <
    typename Hasher = cryp::SipHasher,
    typename HashKey = cryp::SipHashKey,
    typename Hash = cryp::Hash<Hasher, HashKey>
>
class RoutingTree final
{
    struct RoutingNode;
    using HashMap = std::unordered_map<std::string, RoutingNode, Hash, std::equal_to<>>;
    using RootRoutingTree = std::array<HashMap, http::kNumHTTPMethods>;

    struct RoutingNode
    {
        Handler h;
        HashMap hm;

        explicit RoutingNode(const Hash& hasher, Handler handler = nullptr)
            : h(std::move(handler))
            , hm(kDefaultBucketCount, hasher)
        {}
    };

    static constexpr size_t kDefaultBucketCount = 10;

public:
    explicit RoutingTree(const HashKey& key)
        : key_(key)
        , hash_(key_)
        , root_(make_root(hash_))
    {}
    explicit RoutingTree() : RoutingTree(*cryp::make_hash_key<std::tuple_size_v<HashKey>>()) {}

    RoutingTree(const RoutingTree&) = delete;
    RoutingTree& operator=(const RoutingTree&) = delete;

    RoutingTree(RoutingTree&&) noexcept = default;
    RoutingTree& operator=(RoutingTree&&) noexcept = default;

    ~RoutingTree() = default;

    bool add(http::Method mtd, std::string_view path, Handler h);
    Handler get(http::Method mtd, std::string_view path) const noexcept;

private:
    template <size_t... I>
    static RootRoutingTree make_root_impl(const Hash& hasher, std::index_sequence<I...>)
    {
        return RootRoutingTree{ ((void)I, HashMap(kDefaultBucketCount, hasher))... };
    }

    static RootRoutingTree make_root(const Hash& hasher)
    {
        return make_root_impl(hasher, std::make_index_sequence<http::kNumHTTPMethods>{});
    }

    bool set_elem(HashMap& map, std::string_view elem, Handler h);

private:
    HashKey key_;
    Hash hash_;
    RootRoutingTree root_;
};


template <typename Hasher, typename HashKey, typename Hash>
bool RoutingTree<Hasher, HashKey, Hash>::add(http::Method mtd, std::string_view path, Handler h)
{
    size_t idx = static_cast<size_t>(mtd);
    if (idx >= root_.size()) return false; 

    HashMap& map = root_[idx];

    switch (path.length()) {
        case 0:
            return false;
        case 1: {
            if (path[0] != '/') return false;
            return set_elem(map, path, std::move(h));
        }
        default: {
            auto beg = path.begin();
            auto end = path.end();

            auto slash_beg = std::find(beg, end, '/');
            if (slash_beg == end) return false;

            auto slash_end = std::find(slash_beg + 1, end, '/');
            HashMap* target_map = &map;

            while (slash_end != end) {
                std::string_view elem(&*(slash_beg + 1), std::distance(slash_beg + 1, slash_end));

                auto it = target_map->find(elem);
                if (it != target_map->end()) {
                    target_map = &it->second.hm; 
                } else {
                    auto [new_it, inserted] = target_map->try_emplace(
                        std::string(elem), hash_, Handler{nullptr});
                    target_map = &new_it->second.hm;
                }

                slash_beg = slash_end;
                slash_end = std::find(slash_beg + 1, end, '/');
            }

            std::string_view elem(&*(slash_beg + 1), std::distance(slash_beg + 1, slash_end));
            return set_elem(*target_map, elem, std::move(h));
        }
    }
}


template <typename Hasher, typename HashKey, typename Hash>
Handler RoutingTree<Hasher, HashKey, Hash>::get(http::Method mtd, std::string_view path) const noexcept
{
    size_t idx = static_cast<size_t>(mtd);
    if (idx >= root_.size()) return nullptr; 

    const HashMap& map = root_[idx];

    switch (path.length()) {
        case 0:
            return nullptr;
        case 1: {
            auto it = map.find(path);
            if (it == map.end()) return nullptr;
            return it->second.h;
        }
        default: {
            auto beg = path.begin();
            auto end = path.end();

            auto slash_beg = std::find(beg, end, '/');
            if (slash_beg == end) return nullptr;

            auto slash_end = std::find(slash_beg + 1, end, '/');
            const HashMap* target_map = &map;

            while (slash_end != end) {
                std::string_view elem(&*(slash_beg + 1), std::distance(slash_beg + 1, slash_end));

                auto it = target_map->find(elem);
                if (it != target_map->end()) {
                    target_map = &it->second.hm;
                } else {
                    return nullptr;
                }

                slash_beg = slash_end;
                slash_end = std::find(slash_beg + 1, end, '/');
            }
            std::string_view elem(&*(slash_beg + 1), std::distance(slash_beg + 1, slash_end));

            auto res = target_map->find(elem);
            if (res == target_map->end()) return nullptr;
            return res->second.h;
        }
    }
}


template <typename Hasher, typename HashKey, typename Hash>
bool RoutingTree<Hasher, HashKey, Hash>::set_elem(HashMap& map, std::string_view elem, Handler h)
{
    auto it = map.find(elem);
    if (it != map.end()) {
        RoutingNode& rnode = it->second;
        if (rnode.h) return false;
        rnode.h = std::move(h);
        return true;
    }

    map.try_emplace(std::string(elem), hash_, std::move(h));
    return true;
}

} // namespace http

#endif