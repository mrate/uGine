#include "VulkanBumpAllocator.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"

#include <gfxapi/Error.h>

#include <ugine/Align.h>
#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <cassert>

namespace ugine::gfxapi {

GpuAllocation VulkanBumpAllocator::Allocate(size_t size, size_t alignment) {
    current_ = reinterpret_cast<u8*>(AlignTo(reinterpret_cast<size_t>(current_), alignment));

    if (current_ + size > end_) {
        Resize(std::max(size_ * 2, size_ + size * 2));
    }

    auto buffer{ device_->GetStorage().GetBuffer(*buffer_) };
    UGINE_ASSERT(buffer);

    GpuAllocation allocation{
        .buffer = *buffer_,
        .mapped = current_,
        .offset = static_cast<u32>(current_ - reinterpret_cast<u8*>(buffer->mappedData)),
        .size = static_cast<u32>(size),
    };

    current_ += size;

    return allocation;
}

VulkanBumpAllocator::~VulkanBumpAllocator() {
}

void VulkanBumpAllocator::Reset() {
    auto buffer{ device_->GetStorage().GetBuffer(*buffer_) };
    UGINE_ASSERT(buffer);

    current_ = reinterpret_cast<u8*>(buffer->mappedData);
    end_ = current_ + size_;
}

void VulkanBumpAllocator::Resize(size_t size) {
    PROFILE_EVENT();

    // TODO: Where is the deallocation, Lebowski?

    buffer_ = {};

    size_ = size;

    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    };

    bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCI.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkBuffer vkBuffer{};
    VmaAllocation vmaAllocation{};

    VmaAllocationInfo allocInfo{};

    const auto result{ device_->AllocateBuffer(&bufferCI, &allocCI, &vkBuffer, &vmaAllocation, &allocInfo) };

    VulkanBuffer buffer{
        .vkBuffer = vk::Buffer{ vkBuffer },
        .vkMemory = vk::DeviceMemory{ allocInfo.deviceMemory },
        .allocation = vmaAllocation,
        .size = size,
        .mappedData = allocInfo.pMappedData,
    };

    buffer_ = BufferHandleUnique{ device_->GetStorage().EmplaceBuffer(buffer), device_ };

    current_ = reinterpret_cast<u8*>(buffer.mappedData);
    end_ = current_ + size_;
}

void VulkanBumpAllocator::Flush(u32 offset, u32 size) {
    auto buffer{ device_->GetStorage().GetBuffer(*buffer_) };
    UGINE_ASSERT(buffer);

    auto result{ device_->FlushAllocation(buffer->allocation, offset, size) };
    if (result != VK_SUCCESS) {
        UGINE_THROW(GfxError, "Failed to flush VMA");
    }
}

void VulkanBumpAllocator::Flush() {
    auto buffer{ device_->GetStorage().GetBuffer(*buffer_) };
    UGINE_ASSERT(buffer);

    if (current_ != buffer->mappedData) {
        const auto size{ static_cast<u32>(reinterpret_cast<u64>(current_) - reinterpret_cast<u64>(buffer->mappedData)) };
        Flush(0, size);
    }
}

} // namespace ugine::gfxapi
