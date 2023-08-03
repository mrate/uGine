#pragma once

#include <gfxapi/Types.h>

#include <ugine/Hash.h>

#include <vulkan/vulkan.hpp>

namespace ugine::gfxapi {

struct DescriptorLayoutKey {
    std::array<vk::DescriptorSetLayoutBinding, MaxBindings> bindings;
    u32 count{};
};

inline bool operator==(const DescriptorLayoutKey& l, const DescriptorLayoutKey& r) {
    if (l.count != r.count) {
        return false;
    }

    for (u32 i{}; i < l.count; ++i) {
        if (l.bindings[i].binding != r.bindings[i].binding || l.bindings[i].descriptorType != r.bindings[i].descriptorType
            || l.bindings[i].descriptorCount != r.bindings[i].descriptorCount || l.bindings[i].stageFlags != r.bindings[i].stageFlags) {
            return false;
        }
    }
    return true;
}
} // namespace ugine::gfxapi

namespace std {
template <> struct hash<ugine::gfxapi::DescriptorLayoutKey> {
    std::size_t operator()(const ugine::gfxapi::DescriptorLayoutKey& k) const noexcept {
        std::size_t seed{ std::hash<u32>{}(k.count) };
        for (u32 i{}; i < k.count; ++i) {
            ugine::HashCombine(seed, k.bindings[i].binding);
            ugine::HashCombine(seed, k.bindings[i].descriptorCount);
            ugine::HashCombine(seed, u32(k.bindings[i].stageFlags));
        }
        return seed;
    }
};
} // namespace std

namespace ugine::gfxapi {

class VulkanDevice;

class DescriptorSetLayoutCache {
public:
    explicit DescriptorSetLayoutCache(VulkanDevice& device)
        : device_{ device } {}

    vk::DescriptorSetLayout GetOrCreateDescriptorLayout(const vk::DescriptorSetLayoutBinding* bindings, u32 count, bool pushDescriptor);

    size_t Size() { return layouts_.size(); }

    void Clear() { layouts_.clear(); }

private:
    VulkanDevice& device_;
    std::unordered_map<DescriptorLayoutKey, vk::UniqueDescriptorSetLayout> layouts_;
};

} // namespace ugine::gfxapi