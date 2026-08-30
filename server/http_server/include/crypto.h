#ifndef CRYPTO_INCLUDED
#define CRYPTO_INCLUDED

#include <openssl/evp.h>
#include <openssl/core.h>
#include <openssl/core_names.h>
#include <openssl/rand.h>
#include <functional>
#include <optional>
#include <string_view>
#include <array>

#include "http_message.h"


namespace cryp {

struct CTX_Pointer_Free 
{
    void operator()(EVP_MAC_CTX* ctx);
}; 


using SipHashKey = std::array<unsigned char, 16>;
using SipHashTag = std::array<unsigned char, 8>;
using CTX_Pointer = std::unique_ptr<EVP_MAC_CTX, CTX_Pointer_Free>;


std::optional<SipHashKey> make_siphash_key();

template <size_t N>
std::optional<std::array<unsigned char, N>> make_hash_key() 
{
    if constexpr (N == 0) return std::nullopt;
    else {
        std::array<unsigned char, N> key;
        if (RAND_bytes(key.data(), static_cast<int>(key.size())) != 1) {
            return std::nullopt;
        }
        return key;
    }
}


class SipHasher final
{
public:
    explicit SipHasher(const SipHashKey& key);
    SipHasher(const SipHasher&);
    SipHasher(SipHasher&&);
    ~SipHasher() = default;
    SipHasher& operator=(const SipHasher&);
    SipHasher& operator=(SipHasher&&);
public:
    std::optional<SipHashTag> hash(std::string_view msg) const noexcept;
private:
    static CTX_Pointer make_ctx();
    void swap(SipHasher&);
private:
    CTX_Pointer ctx_;
    SipHashKey key_;
};


template <typename Hasher, typename Key>
class Hash
{
public:
    using is_transparent = void;
    explicit Hash(const Key& k) : h_(k), k_(k) {}
    size_t operator()(std::string_view) const noexcept;
    size_t operator()(http::Method) const noexcept;
private:
    Hasher h_;
    Key k_;
};


template <typename Hasher, typename Key>
size_t Hash<Hasher, Key>::operator()(std::string_view msg) const noexcept
{
    auto tag = h_.hash(msg);
    if (!tag) {
        return std::hash<std::string_view>{}(msg);
    }
    size_t result = 0;
    std::memcpy(&result, tag->data(), std::min(sizeof(result), tag->size()));
    return result;
}


template <typename Hasher, typename Key>
size_t Hash<Hasher, Key>::operator()(http::Method v) const noexcept
{
    std::string v_str = http::to_string(v);
    auto tag = h_.hash(v_str);
    if (!tag) {
        return std::hash<std::string_view>{}(v_str);
    }
    size_t result = 0;
    std::memcpy(&result, tag->data(), std::min(sizeof(result), tag->size()));
    return result;
}

} // namespace cryp

#endif 