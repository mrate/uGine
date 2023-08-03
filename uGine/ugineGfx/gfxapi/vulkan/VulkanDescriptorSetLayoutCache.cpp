#include "VulkanDescriptorSetLayoutCache.h"

#include "VulkanDevice.h"

namespace ugine::gfxapi {

vk::DescriptorSetLayout DescriptorSetLayoutCache::GetOrCreateDescriptorLayout(const vk::DescriptorSetLayoutBinding* bindings, u32 count, bool pushDescriptor) {

    vk::DescriptorSetLayoutCreateFlags dsLayoutFlags{};
#ifdef UGINE_VK_PUSH_DESCRIPTORS
    if (pushDescriptor) {
        dsLayoutFlags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;
    }
#endif

    DescriptorLayoutKey key{};
    key.count = count;

    for (u32 i{}; i < count; ++i) {
        key.bindings[i] = bindings[i];
    }

    auto it{ layouts_.find(key) };
    if (it == layouts_.end()) {
        auto layout{ device_.GetDevice().createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo{
            .flags = dsLayoutFlags,
            .bindingCount = count,
            .pBindings = bindings,
        }) };

        it = layouts_.insert(std::make_pair(key, std::move(layout))).first;
    }

    return *it->second;
}

} // namespace ugine::gfxapi