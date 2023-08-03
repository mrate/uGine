#include "FramebufferCache.h"

#include <gfxapi/Device.h>
#include <ugine/Hash.h>

namespace ugine::gfxapi {

FramebufferHandle FramebufferCache::Get(u64 frameNumber, const DynamicFramebuffer& key) {
    auto it{ framebuffers_.find(key) };
    if (it == framebuffers_.end()) {
        FramebufferDesc desc{
            .name = "CachedFB",
            .width = key.extent.width,
            .height = key.extent.height,
            .renderPass = key.renderPass,
            .colorAttachmentCount = key.colorAttachmentCount,
            .depth = key.depth,
        };

        for (u32 i{}; i < key.colorAttachmentCount; ++i) {
            desc.colorAttachments[i] = key.colorAttachments[i];
        }

        auto fb{ device_.CreateFramebufferUnique(desc) };
        it = framebuffers_.insert(std::make_pair(key, std::move(fb))).first;
    }

    return *it->second;
}

void FramebufferCache::PurgeOlderThen(u64 frameNumber) {
    // TODO:
}

} // namespace ugine::gfxapi
