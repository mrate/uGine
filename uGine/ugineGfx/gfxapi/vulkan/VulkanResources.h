#pragma once

#include <gfxapi/Error.h>
#include <gfxapi/Types.h>
#include <gfxapi/vulkan/Vulkan.h>
#include <gfxapi/vulkan/VulkanQueryPool.h>

#include <ugine/SlotMap.h>

namespace ugine::gfxapi {

struct VulkanImage {
    struct ImageView {
        vk::ImageView view{};
        i32 bindlessIndex{ BindlessInvalid };
    };

    vk::Image vkImage;
    vk::DeviceMemory vkMemory{};
    VmaAllocation allocation{};
    TextureDesc desc{};
    vk::ImageAspectFlags vkAspect{ vk::ImageAspectFlagBits::eColor };
    bool ownership{ true };

    // Views.
    ImageView rtv{};
    ImageView colorView{};
    ImageView depthView{};
    ImageView stencilView{};

    vk::ImageView View() const {
        if (vkAspect & vk::ImageAspectFlagBits::eColor) {
            return colorView.view;
        } else if (vkAspect & vk::ImageAspectFlagBits::eDepth) {
            return depthView.view;
        } else {
            return stencilView.view;
        }
    }

    vk::ImageView View(TextureAspectFlags aspect) const {
        switch (aspect) {
        case TextureAspectFlags::Color: return colorView.view;
        case TextureAspectFlags::Depth: return depthView.view;
        case TextureAspectFlags::Stencil: return stencilView.view;
        default:
            UGINE_ASSERT(false);
            UGINE_THROW(GfxError, "Invalid aspect");
            break;
        }
    }

#ifdef UGINE_VK_TRACE_SYNC
    TextureLayout traceLayout{ TextureLayout::Undefined };
#endif
};

struct VulkanFramebuffer {
    vk::UniqueFramebuffer vkFramebuffer;
    vk::RenderPass vkRenderPass{};
    u32 width{};
    u32 height{};
    u32 layers{};

#ifdef UGINE_VK_TRACE_SYNC
    FramebufferDesc desc;
#endif
};

struct VulkanBuffer {
    vk::Buffer vkBuffer{};       // Vulkan buffer.
    vk::DeviceMemory vkMemory{}; // Device memory that backs this buffer.
    VmaAllocation allocation{};  // VMA allocation this buffer origins from (if allocated by VMA).
    vk::DeviceSize size{};       // Allocated size of the buffer.
    void* mappedData{};          // CPU mapped data.

    i32 bindlessIndex{ BindlessInvalid };
};

// Bitmaks specifying which datasets and their bindings are used. Some of the bindings can be optimized away by compiler so it's necessary to ignore them during pipeline binding.
struct VulkanDescriptorBindingMask {
    u8 datasets{};                         // Bitmask specifying which datasets are bound by pipeline.
    std::array<u32, MaxBindings> bindings; // Bitmask specifying which bindings for each dataset are bound by pipeline.
};

struct VulkanPipeline {
    vk::PipelineBindPoint bindPoint{ vk::PipelineBindPoint::eGraphics };
    vk::Pipeline pipeline{};
    vk::PipelineLayout layout{};
    Vector<vk::DescriptorSetLayout> descriptorSetLayouts{};
    Vector<vk::DescriptorSetLayout> descriptorSetDelete{};
    VulkanDescriptorBindingMask bindingMask{};

    u32 pushDescriptorDataset{ MaxDatasets };

    u32 bindlessImagesDataset{ MaxDatasets };
    u32 bindlessSamplerDataset{ MaxDatasets };
    u32 bindlessBuffersDataset{ MaxDatasets };
};

struct VulkanRenderPass {
    vk::UniqueRenderPass vkRenderpass;
    RenderPassDesc desc;
};

struct VulkanBinding {
    u32 set{};
    vk::DescriptorSet descriptorSet{};
};

struct VulkanSampler {
    vk::UniqueSampler sampler;
    i32 bindlessIndex{ BindlessInvalid };
};

class VulkanStorage {
public:
    VulkanStorage() = default;

    VulkanStorage(const VulkanStorage&) = delete;
    VulkanStorage& operator=(const VulkanStorage&) = delete;

    VulkanStorage(VulkanStorage&&) = delete;
    VulkanStorage& operator=(VulkanStorage&&) = delete;

#define VK_STORAGE(Name, Member)                                                                                                                               \
private:                                                                                                                                                       \
    SlotMap<vk::Unique##Name, Name##Handle> Member;                                                                                                            \
                                                                                                                                                               \
public:                                                                                                                                                        \
    template <typename... Args> Name##Handle Emplace##Name(Args&&... args) { return Member.Emplace(std::forward<Args>(args)...); }                             \
    vk::Name Get##Name(Name##Handle handle) const {                                                                                                            \
        auto value{ Member.Get(handle) };                                                                                                                      \
        return value ? **value : nullptr;                                                                                                                      \
    }                                                                                                                                                          \
    void Destroy##Name(Name##Handle handle) { Member.Erase(handle); }

#define STORAGE(Name, Type, Member)                                                                                                                            \
private:                                                                                                                                                       \
    SlotMap<Type, Name##Handle> Member;                                                                                                                        \
                                                                                                                                                               \
public:                                                                                                                                                        \
    template <typename... Args> Name##Handle Emplace##Name(Args&&... args) { return Member.Emplace(std::forward<Args>(args)...); }                             \
    Type* Get##Name(Name##Handle handle) { return Member.Get(handle); }                                                                                        \
    const Type* Get##Name(Name##Handle handle) const { return Member.Get(handle); }                                                                            \
    void Destroy##Name(Name##Handle handle) { Member.Erase(handle); }

    VK_STORAGE(Semaphore, semaphores_)
    VK_STORAGE(Fence, fences_)

    STORAGE(Sampler, VulkanSampler, samplers_)
    STORAGE(RenderPass, VulkanRenderPass, renderPasses_)
    STORAGE(Framebuffer, VulkanFramebuffer, framebuffers_)
    STORAGE(Texture, VulkanImage, textures_)
    STORAGE(Buffer, VulkanBuffer, buffers_)
    STORAGE(GraphicsPipeline, VulkanPipeline, gpos_)
    STORAGE(ComputePipeline, VulkanPipeline, cpos_)
    STORAGE(Binding, VulkanBinding, bindings_)
    STORAGE(QueryPool, VulkanQueryPool, queryPools_)

#undef VK_STORAGE
#undef STORAGE
};

} // namespace ugine::gfxapi