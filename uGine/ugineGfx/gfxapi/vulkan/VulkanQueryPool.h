#pragma once

#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <vulkan/vulkan.hpp>

namespace ugine::gfxapi {

class VulkanDevice;

class VulkanQueryPool {
public:
    VulkanQueryPool(VulkanDevice& device, vk::QueryType type, u32 queryCount);

    void Reset(vk::CommandBuffer cmd);

    void FetchResults();
    u32 Allocate();

    u64 Result(u32 query) const { return values_[query]; }
    Span<const u64> Results() const { return values_.ToSpan(); }

    u32 Count() const { return count_; }

    vk::QueryPool Pool() const { return *pool_; }

private:
    VulkanDevice& device_;

    const u32 count_{};
    u32 queryCounter_{};
    vk::UniqueQueryPool pool_;

    Vector<u64> values_;
};

} // namespace ugine::gfxapi