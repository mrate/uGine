#pragma once

#include <ugine/Memory.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <vulkan/vulkan.hpp>

#include <optional>

namespace ugine::gfxapi {

class VulkanDevice;

class VulkanBindlessPool {
public:
    VulkanBindlessPool(VulkanDevice& device, vk::DescriptorType type, u32 maxResources, IAllocator& allocator = IAllocator::Default());
    ~VulkanBindlessPool();

    vk::DescriptorSet Descriptor() const { return descriptor_; }
    vk::DescriptorSetLayout Layout() const { return *layout_; }

    void SetNull(const vk::DescriptorImageInfo& imageInfo) { return SetNull(imageInfo, {}); }
    void SetNull(const vk::DescriptorBufferInfo& bufferInfo) { return SetNull({}, bufferInfo); }

    i32 Allocate(const vk::DescriptorImageInfo& imageInfo) { return Allocate(imageInfo, {}); }
    i32 Allocate(const vk::DescriptorBufferInfo& bufferInfo) { return Allocate({}, bufferInfo); }
    void Free(i32 bindlessIndex);

private:
    i32 Allocate(const vk::DescriptorImageInfo& imageInfo, const vk::DescriptorBufferInfo& bufferInfo);
    void SetNull(const vk::DescriptorImageInfo& imageInfo, const vk::DescriptorBufferInfo& bufferInfo);

    VulkanDevice& device_;
    const vk::DescriptorType type_;
    const u32 maxResources_;

    u32 binding_{ 0 }; // TODO:

    vk::UniqueDescriptorSetLayout layout_;
    vk::UniqueDescriptorPool pool_;
    vk::DescriptorSet descriptor_;

    std::optional<vk::DescriptorImageInfo> nullImageInfo_{};
    std::optional<vk::DescriptorBufferInfo> nullBufferInfo_{};

    Vector<i32> freeList_;
};

} // namespace ugine::gfxapi