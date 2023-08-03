#include "RenderTargetCache.h"

#include "Device.h"

#include <format>

namespace ugine::gfxapi {

TextureHandle RenderTargetCache::Get(u64 frameNumber, const Extent2D& extent, Format format, TextureUsageFlags usage) {
    const RtvDesc desc{
        .width = extent.width,
        .height = extent.height,
        .format = format,
        .usage = usage,
    };

    auto it{ rtvs_.find(desc) };
    if (it == rtvs_.end()) {
        std::vector<CachedRtv> cache{};
        cache.reserve(16);
        it = rtvs_.insert(std::make_pair(desc, std::move(cache))).first;
    } else if (frameNumber > framesInFlight_) {
        for (auto& texture : it->second) {
            if (texture.frameNumber < frameNumber - framesInFlight_) {
                texture.frameNumber = frameNumber;
                return *texture.texture;
            }
        }
    }

    return AddTexture(it->second, frameNumber, extent, format, usage);
}

TextureHandle RenderTargetCache::AddTexture(std::vector<CachedRtv>& cache, u64 frameNumber, const Extent2D& extent, Format format, TextureUsageFlags usage) {
    static u32 counter{};

    const TextureDesc rtvDesc{
        .name = std::format("RtvCacheTexture_{}x{}/{}({})", extent.width, extent.height, int(format), ++counter),
        .extent = extent,
        .format = format,
        .usage = usage,
    };

    cache.emplace_back(frameNumber, device_.CreateTextureUnique(rtvDesc, TextureLayout::Undefined));
    return *cache.back().texture;
}

void RenderTargetCache::PurgeOlderThen(u64 frameNumber) {
    // TODO:
}

} // namespace ugine::gfxapi