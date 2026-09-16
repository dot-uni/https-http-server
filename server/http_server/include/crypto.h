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
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <span>

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
std::shared_ptr<SipHashKey> make_siphash_key_ptr();

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


template <size_t N>
std::shared_ptr<std::array<unsigned char, N>> make_hash_key_ptr() 
{
    using byte_array = std::array<unsigned char, N>;

    if constexpr (N == 0) return nullptr;
    else {
        auto key = std::make_shared<byte_array>();
        if (RAND_bytes(key->data(), static_cast<int>(N)) != 1) {
            return nullptr;
        }
        return key;
    }
}


template <typename Derived>
class Hash 
{
public:
    using is_transparent = void;
    
    virtual ~Hash() = default;

    [[nodiscard]] virtual std::optional<std::vector<unsigned char>> hash(std::string_view) const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<Hash> clone() const = 0;

    template <typename T> 
    requires std::is_trivially_copyable_v<T>
    size_t operator()(const T&) const noexcept;
    size_t operator()(std::string_view v) const noexcept;
protected:
    Hash() = default;

    template <typename T>
    size_t tag_to_size_t(const T* ptag, size_t size) const;
};


template <typename Derived>
template <typename T> 
requires std::is_trivially_copyable_v<T>
size_t Hash<Derived>::operator()(const T& v) const noexcept
{
    static_assert(std::is_base_of_v<Hash<Derived>, Derived>);

    const auto bytes = std::as_bytes(std::span<const T>(&v, 1));
    auto msg = std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());

    const auto* self = static_cast<const Derived*>(this);

    if constexpr (requires { self->fhash(msg); }) {
        auto tag = self->fhash(msg);
        if (!tag) return 0;
        return tag_to_size_t((*tag).data(), (*tag).size());
    }
    else {
        auto tag = self->hash(msg);
        if (!tag) return 0;
        return tag_to_size_t((*tag).data(), (*tag).size());
    }
}


template <typename Derived>
size_t Hash<Derived>::operator()(std::string_view v) const noexcept
{
    static_assert(std::is_base_of_v<Hash<Derived>, Derived>);

    const auto* self = static_cast<const Derived*>(this);

    if constexpr (requires { self->fhash(v); }) {
        auto tag = self->fhash(v);
        if (!tag) return 0;
        return tag_to_size_t((*tag).data(), (*tag).size());
    }
    else {
        auto tag = self->hash(v);
        if (!tag) return 0;
        return tag_to_size_t((*tag).data(), (*tag).size());
    }
}


template <typename Derived>
template <typename T>
size_t Hash<Derived>::tag_to_size_t(const T* ptag, size_t size) const
{   
    size_t result = 0;
    std::memcpy(&result, ptag, std::min(size, sizeof(result)));
    return result;
}


class SipHash final : public Hash<SipHash>
{
public:
    using Key = SipHashKey;

    SipHash() : key_(make_siphash_key_ptr()) {}
    explicit SipHash(std::shared_ptr<SipHashKey> key);
    SipHash(const SipHash&);
    SipHash(SipHash&&);
    ~SipHash() = default;
    SipHash& operator=(const SipHash&);
    SipHash& operator=(SipHash&&);
public:
    std::optional<SipHashTag> fhash(std::string_view v) const noexcept;
    std::optional<std::vector<unsigned char>> hash(std::string_view v) const noexcept override;
    std::unique_ptr<Hash> clone() const override;
private:
    static CTX_Pointer make_ctx();
    void swap(SipHash&);
private:
    CTX_Pointer ctx_;
    std::shared_ptr<SipHashKey> key_;
};

} // namespace cryp

#endif 