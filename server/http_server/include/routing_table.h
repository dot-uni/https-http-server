#ifndef ROUTING_TABLE_INCLUDED
#define ROUTING_TABLE_INCLUDED

#include <openssl/evp.h>
#include <vector>

#include "hash.h"

namespace http {

template <
    typename Key, 
    typename Val, 
    typename Hasher = hash::SipHasher
> class RoutingTable 
{
public:
    template <size_t N = std::tuple_size_v<hash::SipHashKey>>
    constexpr RoutingTable();

    ~RoutingTable() = default;
    Val& operator[](const Key& k); 
public:
    void insert();
    constexpr bool empty() const noexcept;
    constexpr size_t size() const noexcept;
protected:
    
protected:
    Hasher hash_;
    std::vector<Val> data_;
};


template <typename Key, typename Val, typename Hasher>
template <size_t N>
constexpr RoutingTable<Key, Val, Hasher>::RoutingTable()
{
    auto key = hash::make_hash_key<N>();
    if (!key) throw std::runtime_error("The hash key was not created");
    hash_(*key);
}


template <typename Key, typename Val, typename Hasher>
constexpr bool RoutingTable<Key, Val, Hasher>::empty() const noexcept { return data_.empty(); }

template <typename Key, typename Val, typename Hasher>
constexpr size_t RoutingTable<Key, Val, Hasher>::size() const noexcept { return data_.size(); }



} // namespace http

#endif
