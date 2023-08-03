#pragma once

#include <gfxapi/vulkan/Vulkan.h>

namespace ugine::gfxapi {

class VulkanDevice;

class VulkanDescriptorSetPool {
public:
    VulkanDescriptorSetPool(VulkanDevice& device, const std::vector<vk::DescriptorPoolSize>& sizes, u32 singlePoolSize = 512u);
    ~VulkanDescriptorSetPool();

    VulkanDescriptorSetPool(const VulkanDescriptorSetPool&) = delete;
    VulkanDescriptorSetPool& operator=(const VulkanDescriptorSetPool&) = delete;

    VulkanDescriptorSetPool(VulkanDescriptorSetPool&&) = default;
    VulkanDescriptorSetPool& operator=(VulkanDescriptorSetPool&&) = default;

    [[nodiscard]] void CreatePool();

    [[nodiscard]] vk::DescriptorSet Allocate(vk::DescriptorSetLayout layout);
    void Free(vk::DescriptorSet ds);

    void Reset();

private:
    void DestroyPool();

    VulkanDevice& device_;
    vk::UniqueDescriptorPool pool_;

    const std::vector<vk::DescriptorPoolSize> sizes_;
    const u32 singlePoolSize_{};

    u32 allocated_{};
};

} // namespace ugine::gfxapi