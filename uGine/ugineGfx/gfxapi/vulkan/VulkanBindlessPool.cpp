#include "VulkanBindlessPool.h"
#include "VulkanDevice.h"

namespace ugine::gfxapi {

VulkanBindlessPool::VulkanBindlessPool(VulkanDevice& device, vk::DescriptorType type, u32 maxResources, IAllocator& allocator)
    : device_{ device }
    , type_{ type }
    , maxResources_{ maxResources }
    , freeList_{ allocator } {

    UGINE_ASSERT(maxResources > 0);
    UGINE_ASSERT(maxResources < 500000u);

    // Layout.
    vk::DescriptorBindingFlags bindingFlags{};
    bindingFlags |= vk::DescriptorBindingFlagBits::ePartiallyBound;
    bindingFlags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
    bindingFlags |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
    //bindingFlags |= vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;

    vk::DescriptorSetLayoutBindingFlagsCreateInfo layoutBindingCI{
        .bindingCount = 1,
        .pBindingFlags = &bindingFlags,
    };

    vk::DescriptorSetLayoutBinding binding{
        .binding = binding_,
        .descriptorType = type,
        .descriptorCount = u32(maxResources),
        .stageFlags = vk::ShaderStageFlagBits::eAll,
    };

    vk::DescriptorSetLayoutCreateInfo layoutCI{
        .pNext = &layoutBindingCI,
        .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
        .bindingCount = 1,
        .pBindings = &binding,
    };

    layout_ = device.GetDevice().createDescriptorSetLayoutUnique(layoutCI);

    // Pool.

    vk::DescriptorPoolSize poolSize = { vk::DescriptorType::eSampledImage, maxResources };
    vk::DescriptorPoolCreateInfo poolCI{
        .flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };

    pool_ = device.GetDevice().createDescriptorPoolUnique(poolCI);

    // Allocate descriptor.
    vk::DescriptorSetLayout layout{ *layout_ };

    const u32 bindingCount{ maxResources - 1 };

    vk::DescriptorSetVariableDescriptorCountAllocateInfo countInfo{
        .descriptorSetCount = 1,
        .pDescriptorCounts = &bindingCount,
    };

    vk::DescriptorSetAllocateInfo allocInfo{
        .pNext = &countInfo,
        .descriptorPool = *pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    descriptor_ = device.GetDevice().allocateDescriptorSets(allocInfo)[0];

    // Freelist.

    for (i32 i{ i32(maxResources) }; i > 0; --i) {
        freeList_.PushBack(i - 1);
    }
}

VulkanBindlessPool::~VulkanBindlessPool() {
    device_.GetDevice().resetDescriptorPool(*pool_);
    pool_ = {};
    layout_ = {};
}

i32 VulkanBindlessPool::Allocate(const vk::DescriptorImageInfo& imageInfo, const vk::DescriptorBufferInfo& bufferInfo) {
    if (freeList_.Empty()) {
        return BindlessInvalid;
    }

    const auto index{ freeList_.Back() };
    freeList_.PopBack();

    vk::WriteDescriptorSet writes{
        .dstSet = descriptor_,
        .dstBinding = binding_,
        .dstArrayElement = u32(index),
        .descriptorCount = 1,
        .descriptorType = type_,
        .pImageInfo = &imageInfo,
        .pBufferInfo = &bufferInfo,
    };

    device_.GetDevice().updateDescriptorSets(1, &writes, 0, nullptr);

    return index;
}

void VulkanBindlessPool::SetNull(const vk::DescriptorImageInfo& imageInfo, const vk::DescriptorBufferInfo& bufferInfo) {
    nullImageInfo_ = imageInfo;
    nullBufferInfo_ = bufferInfo;

    Vector<vk::WriteDescriptorSet> writes{ maxResources_, device_.Allocator() };
    for (u32 i{}; i < freeList_.Size(); ++i) {
        writes[i] = vk::WriteDescriptorSet{
            .dstSet = descriptor_,
            .dstBinding = binding_,
            .dstArrayElement = u32(freeList_[i]),
            .descriptorCount = 1,
            .descriptorType = type_,
            .pImageInfo = &nullImageInfo_.value(),
            .pBufferInfo = &nullBufferInfo_.value(),
        };
    }

    device_.GetDevice().updateDescriptorSets(u32(freeList_.Size()), writes.Data(), 0, nullptr);
}

void VulkanBindlessPool::Free(i32 bindlessIndex) {
    UGINE_ASSERT(bindlessIndex != BindlessInvalid);

    if (nullImageInfo_ || nullBufferInfo_) {
        vk::WriteDescriptorSet writes{
            .dstSet = descriptor_,
            .dstBinding = binding_,
            .dstArrayElement = u32(bindlessIndex),
            .descriptorCount = 1,
            .descriptorType = type_,
            .pImageInfo = &nullImageInfo_.value(),
            .pBufferInfo = &nullBufferInfo_.value(),
        };
        device_.GetDevice().updateDescriptorSets(1, &writes, 0, nullptr);
    }

    freeList_.PushBack(bindlessIndex);
}

} // namespace ugine::gfxapi