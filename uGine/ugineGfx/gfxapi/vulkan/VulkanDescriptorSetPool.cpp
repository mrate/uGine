#include "VulkanDescriptorSetPool.h"
#include "VulkanDevice.h"

//#include <ugine/engine/gfx/GraphicsState.h>
//#include <ugine/engine/gfx/core/Device.h>

#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <iterator>

namespace ugine::gfxapi {

DescriptorSetPool::DescriptorSetPool(VulkanDevice& device, const std::vector<vk::DescriptorPoolSize>& sizes, u32 singlePoolSize, bool allowFree)
    : device_{ device }
    , sizes_{ sizes }
    , singlePoolSize_{ singlePoolSize }
    , allowFree_{ allowFree } {
    CreatePool();
}

DescriptorSetPool::~DescriptorSetPool() {
    DestroyPool();
}

vk::DescriptorSet DescriptorSetPool::Allocate(vk::DescriptorSetLayout layout) {
    //if (layout == layout_ && allocatedCounter_ > 0 && allocatedCounter_ < allocatedDs_.size()) {
    //    return allocatedDs_[allocatedCounter_++];
    //} else {
    PROFILE_EVENT();

    layout_ = layout;
    allocatedCounter_ = 0;

    VkDescriptorSetLayout vkLayout{ layout };

    do {
        //std::vector<VkDescriptorSetLayout> layouts(singlePoolSize_, vkLayout);

        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = *pool_,
            .descriptorSetCount = 1,
            .pSetLayouts = &vkLayout,
        };

        // vk::allocate creates vector...
        auto res{ vkAllocateDescriptorSets(device_.GetDevice(), &allocInfo, allocatedDs_.data()) };
        if (res == VK_SUCCESS) {
            return allocatedDs_[allocatedCounter_++];
        }

        DestroyPool();
        singlePoolSize_ *= 2;
        CreatePool();
    } while (true);
    //}
}

void DescriptorSetPool::Free(vk::DescriptorSet ds) {
    UGINE_ASSERT(allowFree_);
    device_.GetDevice().freeDescriptorSets(*pool_, ds);
}

void DescriptorSetPool::Reset() {
    device_.GetDevice().resetDescriptorPool(*pool_, {});
}

void DescriptorSetPool::DestroyPool() {
    // TODO: If single free is available then other strategy needs to be implemented.
    //UGINE_ASSERT(!allowFree_);

    device_.DeferredDestroy(std::move(pool_));
}

void DescriptorSetPool::CreatePool() {
    vk::DescriptorPoolCreateFlags flags{};
    if (allowFree_) {
        flags |= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    }

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = flags,
        .maxSets = singlePoolSize_,
        .poolSizeCount = static_cast<u32>(sizes_.size()),
        .pPoolSizes = sizes_.data(),
    };

    pool_ = device_.GetDevice().createDescriptorPoolUnique(poolInfo);

    allocatedCounter_ = 0;
    allocatedDs_.resize(singlePoolSize_);
}

} // namespace ugine::gfxapi
