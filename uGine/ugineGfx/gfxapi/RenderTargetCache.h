#pragma once

#include "Handle.h"
#include "Types.h"

#include <ugine/Hash.h>

#include <unordered_map>

namespace ugine::gfxapi {

class Device;

struct RtvDesc {
    u32 width{};
    u32 height{};
    Format format{};
    TextureUsageFlags usage{};
};

inline bool operator==(const RtvDesc& l, const RtvDesc& r) {
    return l.width == r.width && l.height == r.height && l.format == r.format && l.usage == r.usage;
}

} // namespace ugine::gfxapi

namespace std {
template <> struct hash<ugine::gfxapi::RtvDesc> {
    std::size_t operator()(const ugine::gfxapi::RtvDesc& d) const noexcept {
        std::size_t seed{};
        ugine::HashCombine(seed, d.width);
        ugine::HashCombine(seed, d.height);
        ugine::HashCombine(seed, d.format);
        ugine::HashCombine(seed, d.usage);
        return seed;
    }
};
} // namespace std

namespace ugine::gfxapi {

class RenderTargetCache {
public:
    RenderTargetCache(Device& device, u32 framesInFlight)
        : device_{ device }
        , framesInFlight_{ framesInFlight } {}

    TextureHandle Get(
        u64 frameNumber, const Extent2D& extent, Format format, TextureUsageFlags usage = TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled);

    size_t Size() const { return rtvs_.size(); }
    void PurgeOlderThen(u64 frameNumber);

private:
    struct CachedRtv {
        u64 frameNumber; // Last frame texture was used.
        TextureHandleUnique texture;
    };

    TextureHandle AddTexture(std::vector<CachedRtv>& cache, u64 frameNumber, const Extent2D& extent, Format format, TextureUsageFlags usage);

    Device& device_;
    u32 framesInFlight_{};
    std::unordered_map<RtvDesc, std::vector<CachedRtv>> rtvs_;
};

} // namespace ugine::gfxapi