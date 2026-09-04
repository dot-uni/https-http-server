#include "crypto.h"

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


SipHasher::SipHasher(const SipHashKey& key) : key_(key)
{
    ctx_ = make_ctx();
    if (!ctx_) throw std::runtime_error("The context was not created");
}


SipHasher::SipHasher(const SipHasher& h)
{
    key_ = h.key_;
    ctx_ = make_ctx();
    if (!ctx_) throw std::runtime_error("The context was not created");
}


SipHasher::SipHasher(SipHasher&& h) 
{
    ctx_ = std::move(h.ctx_);
    key_ = std::move(h.key_);
}


SipHasher& SipHasher::operator=(const SipHasher& h)
{
    if (&h == this) return *this;

    key_ = h.key_;
    ctx_ = make_ctx();
    if (!ctx_) throw std::runtime_error("The context was not created");
    
    return *this;
}


SipHasher& SipHasher::operator=(SipHasher&& h)
{
    if (&h == this) return *this;
    ctx_ = std::move(h.ctx_);
    key_ = std::move(h.key_);
    return *this;
}


std::optional<SipHashTag> SipHasher::hash(std::string_view msg) const noexcept
{
    SipHashTag tag{};

    size_t tag_size = 0;
    bool ok = 
        EVP_MAC_init(ctx_.get(), key_.data(), key_.size(), nullptr) == 1 &&
        EVP_MAC_update(ctx_.get(), reinterpret_cast<const unsigned char*>(msg.data()), msg.size()) == 1 &&
        EVP_MAC_final(ctx_.get(), tag.data(), &tag_size, tag.size()) == 1 &&
        tag_size == tag.size();
    if (!ok) {
        return std::nullopt;
    }
    return tag;
}


CTX_Pointer SipHasher::make_ctx()
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


void SipHasher::swap(SipHasher& h)
{
    std::swap(ctx_, h.ctx_);
    std::swap(key_, h.key_);
}

} // namespace crypto