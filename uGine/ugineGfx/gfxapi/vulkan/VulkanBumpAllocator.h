#pragma once

#include <gfxapi/BumpAllocator.h>
#include <gfxapi/vulkan/Vulkan.h>

namespace ugine::gfxapi {

class VulkanDevice;

class VulkanBumpAllocator final : public BumpAllocator {
public:
    UGINE_NO_COPY(VulkanBumpAllocator);

    explicit VulkanBumpAllocator(VulkanDevice& device)
        : device_{ &device } {}

    VulkanBumpAllocator() = default;
    ~VulkanBumpAllocator();

    VulkanBumpAllocator(VulkanBumpAllocator&& other) {
        std::swap(device_, other.device_);
        std::swap(size_, other.size_);
        std::swap(buffer_, other.buffer_);
        std::swap(current_, other.current_);
        std::swap(end_, other.end_);
    }

    VulkanBumpAllocator& operator=(VulkanBumpAllocator&& other) {
        std::swap(device_, other.device_);
        std::swap(size_, other.size_);
        std::swap(buffer_, other.buffer_);
        std::swap(current_, other.current_);
        std::swap(end_, other.end_);
        return *this;
    }

    void Resize(size_t size);

    void Flush(u32 offset, u32 size);
    void Flush();

    void Reset();
    GpuAllocation Allocate(size_t size, size_t alignment) override;

private:
    VulkanDevice* device_{};
    BufferHandleUnique buffer_{};
    size_t size_{};

    u8* current_{};
    u8* end_{};
};

} // namespace ugine::gfxapi
