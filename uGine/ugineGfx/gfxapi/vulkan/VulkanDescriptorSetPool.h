#pragma once

#include <ugine/Ugine.h>

#include <vulkan/vulkan.hpp>

#include <deque>

namespace ugine::gfxapi {

class VulkanDevice;

class DescriptorSetPool {
public:
    DescriptorSetPool(VulkanDevice& device, const std::vector<vk::DescriptorPoolSize>& sizes, u32 singlePoolSize = 512u, bool allowFree = false);
    ~DescriptorSetPool();

    [[nodiscard]] vk::DescriptorSet Allocate(vk::DescriptorSetLayout layout);
    void Free(vk::DescriptorSet ds);

    void Reset();

private:
    void CreatePool();
    void DestroyPool();

    VulkanDevice& device_;
    vk::UniqueDescriptorPool pool_;
    const std::vector<vk::DescriptorPoolSize> sizes_;
    u32 singlePoolSize_{};
    bool allowFree_{};

    // TODO:
    std::vector<VkDescriptorSet> allocatedDs_;
    u32 allocatedCounter_{};
    vk::DescriptorSetLayout layout_{};
};

} // namespace ugine::gfxapi
