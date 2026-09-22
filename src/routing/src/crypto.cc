#include "project/routing/crypto.h"


namespace uni {
namespace cryp {

void CTX_Pointer_Free::operator()(EVP_MAC_CTX* ctx)
{
    EVP_MAC_CTX_free(ctx);
}


std::optional<SipHashKey> make_siphash_key()
{
    SipHashKey key{};
    if (RAND_bytes(key.data(), static_cast<int>(key.size())) != 1) {
        return std::nullopt;
    }
    return key;
}


std::shared_ptr<SipHashKey> make_siphash_key_ptr()
{
    auto key = std::make_shared<SipHashKey>();
    if (RAND_bytes(key->data(), static_cast<int>(key->size())) != 1) {
        return nullptr;
    }
    return key;
}


SipHash::SipHash(std::shared_ptr<SipHashKey> key) : ctx_(make_ctx()), key_(std::move(key))
{
    ctx_ = make_ctx();
    if (!ctx_) throw std::runtime_error("The context was not created");
}


SipHash::SipHash(const SipHash& h)
{
    key_ = h.key_;
    ctx_ = make_ctx();
    if (!ctx_) throw std::runtime_error("The context was not created");
}


SipHash::SipHash(SipHash&& h) 
{
    ctx_ = std::move(h.ctx_);
    key_ = std::move(h.key_);
}


SipHash& SipHash::operator=(const SipHash& h)
{
    if (&h == this) return *this;

    key_ = h.key_;
    ctx_ = make_ctx();
    if (!ctx_) throw std::runtime_error("The context was not created");
    
    return *this;
}


SipHash& SipHash::operator=(SipHash&& h)
{
    if (&h == this) return *this;
    ctx_ = std::move(h.ctx_);
    key_ = std::move(h.key_);
    return *this;
}


std::optional<SipHashTag> SipHash::fhash(std::string_view v) const noexcept
{
    SipHashTag tag{};

    size_t tag_size = 0;
    bool ok = 
        EVP_MAC_init(ctx_.get(), (*key_).data(), (*key_).size(), nullptr) == 1 &&
        EVP_MAC_update(ctx_.get(), reinterpret_cast<const unsigned char*>(v.data()), v.size()) == 1 &&
        EVP_MAC_final(ctx_.get(), tag.data(), &tag_size, tag.size()) == 1 &&
        tag_size == tag.size();
    if (!ok) {
        return std::nullopt;
    }
    return tag;
}


std::optional<std::vector<unsigned char>> SipHash::hash(std::string_view v) const noexcept
{
    auto tag = fhash(v);
    if (!tag) {
        return std::nullopt;
    }
    return std::vector<unsigned char>(tag->begin(), tag->end());
}


std::unique_ptr<Hash<SipHash>> SipHash::clone() const
{
    return std::make_unique<SipHash>(*this);
}


CTX_Pointer SipHash::make_ctx()
{
    EVP_MAC* mac = EVP_MAC_fetch(nullptr, "SIPHASH", nullptr);
    if (!mac) { return nullptr; }

    CTX_Pointer ctx(EVP_MAC_CTX_new(mac));
    EVP_MAC_free(mac);
    if (!ctx) { return nullptr; }

    size_t tag_size = 8;
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_size_t(OSSL_MAC_PARAM_SIZE, &tag_size),
        OSSL_PARAM_construct_end()
    };

    if (EVP_MAC_CTX_set_params(ctx.get(), params) != 1) {
        return nullptr;
    }
    return ctx;
}


void SipHash::swap(SipHash& h)
{
    std::swap(ctx_, h.ctx_);
    std::swap(key_, h.key_);
}

} // namespace cryp
} // namespace uni