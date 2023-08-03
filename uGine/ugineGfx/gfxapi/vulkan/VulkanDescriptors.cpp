#include "VulkanDescriptors.h"
#include "VulkanDevice.h"

namespace ugine::gfxapi {

VulkanDescriptorSetPool::VulkanDescriptorSetPool(VulkanDevice& device, const std::vector<vk::DescriptorPoolSize>& sizes, u32 singlePoolSize)
    : device_{ device }
    , sizes_{ sizes }
    , singlePoolSize_{ singlePoolSize } {
}

VulkanDescriptorSetPool::~VulkanDescriptorSetPool() {
    DestroyPool();
}

vk::DescriptorSet VulkanDescriptorSetPool::Allocate(vk::DescriptorSetLayout layout) {
    // Don't use vk:: here to avoid use of std::vector.
    ++allocated_;

    VkDescriptorSetLayout dsLayout{ layout };
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = static_cast<VkDescriptorPool>(*pool_);
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsLayout;

    VkDescriptorSet descriptorSet{};
    const auto result{ vkAllocateDescriptorSets(static_cast<VkDevice>(device_.GetDevice()), &allocInfo, &descriptorSet) };
    return vk::DescriptorSet{ descriptorSet };
}

void VulkanDescriptorSetPool::Free(vk::DescriptorSet ds) {
    device_.GetDevice().freeDescriptorSets(*pool_, ds);
}

void VulkanDescriptorSetPool::Reset() {
    if (allocated_) {
        allocated_ = 0;
        device_.GetDevice().resetDescriptorPool(*pool_, {});
    }
}

void VulkanDescriptorSetPool::DestroyPool() {
    pool_ = {};
    allocated_ = 0;
}

void VulkanDescriptorSetPool::CreatePool() {
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.pPoolSizes = sizes_.data();
    poolInfo.poolSizeCount = static_cast<u32>(sizes_.size());
    poolInfo.maxSets = singlePoolSize_;

    pool_ = device_.GetDevice().createDescriptorPoolUnique(poolInfo);
}

} // namespace ugine::gfxapi