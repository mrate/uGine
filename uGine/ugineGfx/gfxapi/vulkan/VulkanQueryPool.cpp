#include "VulkanQueryPool.h"
#include "VulkanDevice.h"

namespace ugine::gfxapi {

VulkanQueryPool::VulkanQueryPool(VulkanDevice& device, vk::QueryType type, u32 queryCount)
    : device_{ device }
    , count_{ queryCount }
    , values_(queryCount, 0) {
    vk::QueryPoolCreateInfo queryPoolInfo{
        .queryType = type,
        .queryCount = queryCount,
    };

    pool_ = device.GetDevice().createQueryPoolUnique(queryPoolInfo);
}

void VulkanQueryPool::Reset(vk::CommandBuffer cmd) {
    cmd.resetQueryPool(*pool_, 0, count_);
    queryCounter_ = 0;
}

void VulkanQueryPool::FetchResults() {
    if (queryCounter_) {
        vkGetQueryPoolResults(device_.GetDevice(), *pool_, 0, queryCounter_, sizeof(u64) * queryCounter_, values_.Data(), sizeof(u64), VK_QUERY_RESULT_64_BIT);
    }
}

u32 VulkanQueryPool::Allocate() {
    const auto query{ queryCounter_++ };
    UGINE_ASSERT(query < count_);
    return query;
}

} // namespace ugine::gfxapi