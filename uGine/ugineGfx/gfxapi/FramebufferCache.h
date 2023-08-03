#pragma once

#include <gfxapi/Types.h>

#include <ugine/Hash.h>

#include <array>
#include <unordered_map>

namespace ugine::gfxapi {

class Device;

inline bool operator==(const DynamicFramebuffer& l, const DynamicFramebuffer& r) {
    if (l.renderPass != r.renderPass || l.depth != r.depth || l.colorAttachmentCount != r.colorAttachmentCount || l.extent.width != r.extent.width
        || l.extent.height != r.extent.height) {
        return false;
    }

    for (u32 i{}; i < l.colorAttachmentCount; ++i) {
        if (l.colorAttachments[i] != r.colorAttachments[i]) {
            return false;
        }
    }

    return true;
}

} // namespace ugine::gfxapi

namespace std {
template <> struct hash<ugine::gfxapi::DynamicFramebuffer> {
    std::size_t operator()(const ugine::gfxapi::DynamicFramebuffer& k) const noexcept {
        std::size_t seed{ std::hash<u32>{}(k.colorAttachmentCount) };
        ugine::HashCombine(seed, k.renderPass.m_value);
        ugine::HashCombine(seed, k.depth.m_value);
        for (u32 i{}; i < k.colorAttachmentCount; ++i) {
            ugine::HashCombine(seed, k.colorAttachments[i].m_value);
        }
        return seed;
    }
};
} // namespace std

namespace ugine::gfxapi {
class FramebufferCache {
public:
    FramebufferCache(Device& device, u32 framesInFlight)
        : device_{ device }
        , framesInFlight_{ framesInFlight } {}

    FramebufferHandle Get(u64 frameNumber, const DynamicFramebuffer& key);

    size_t Size() const { return framebuffers_.size(); }
    void PurgeOlderThen(u64 frameNumber);

private:
    Device& device_;
    u32 framesInFlight_{};
    std::unordered_map<DynamicFramebuffer, FramebufferHandleUnique> framebuffers_;
};

} // namespace ugine::gfxapi
