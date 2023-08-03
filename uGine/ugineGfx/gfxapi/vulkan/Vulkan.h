#pragma once

#include <gfxapi/Types.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#define VMA_VULKAN_VERSION 1003000
#include <vk_mem_alloc.h>

namespace ugine {
struct ColorRGBA;
}

namespace ugine::gfxapi {
struct ClearValue;
} // namespace ugine::gfxapi

namespace ugine::gfxapi {

struct SwapChainCapabilities {
    vk::SurfaceCapabilitiesKHR capabilities{};
    std::vector<vk::SurfaceFormatKHR> formats{};
    std::vector<vk::PresentModeKHR> presentModes{};
};

struct QueueFamilies {
    u32 graphics{};
    u32 compute{};
    u32 transfer{};
    u32 surfacePresent{};
};

struct DeviceDescriptor {
    vk::PhysicalDevice device{};
    vk::SurfaceKHR surface{};
    QueueFamilies queueFamilies{};
};

DeviceDescriptor GetFirstSuitableDevice(vk::Instance instance, vk::SurfaceKHR surface, const std::vector<const char*> requiredExtensions);
SwapChainCapabilities QuerySwapChainCapabilities(vk::PhysicalDevice device, vk::SurfaceKHR surface);
vk::SampleCountFlagBits GetMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);
vk::Format FindSupportedFormat(
    vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
Format FindSupportedFormat(vk::PhysicalDevice& physicalDevice, const std::vector<Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
QueueFamilies GetDeviceQueues(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

bool FormatHasDepth(vk::Format format);
bool FormatHasStencil(vk::Format format);

vk::ImageAspectFlags AspectFromFormat(vk::Format format);

// Conversions.
vk::ClearValue ToVulkanColorClearValue(const ClearValue& color);
vk::ClearValue ToVulkanDepthStencilClearValue(const ClearValue& color);

Format FromVulkan(vk::Format format);

vk::Format ToVulkan(Format format);
vk::AttachmentLoadOp ToVulkan(AttachmentLoadOp op);
vk::AttachmentStoreOp ToVulkan(AttachmentStoreOp op);
vk::AttachmentDescription ToVulkan(const RenderPassAttachmentDesc& desc);
vk::IndexType ToVulkan(IndexType indexType);
vk::ImageLayout ToVulkan(TextureLayout layout);
vk::SamplerAddressMode ToVulkan(const TextureAddressMode& mode);
vk::Filter ToVulkan(Filter filter);
vk::CompareOp ToVulkan(ComparisonFunc func);
vk::VertexInputAttributeDescription ToVulkan(const VertexAttribute& desc);
vk::CullModeFlagBits ToVulkan(CullMode cullMode);
vk::PolygonMode ToVulkan(FillMode fillMode);
vk::QueryType ToVulkan(QueryType queryType);
vk::PipelineStageFlags2 ToVulkan(PipelineStage stage);

vk::BufferCopy ToVulkan(const BufferCopy& copy);
vk::StencilFaceFlags ToVulkan(StencilFaceFlags flags);

vk::PipelineInputAssemblyStateCreateInfo ToVulkan(const InputAssemblyDesc& desc);
vk::PipelineDepthStencilStateCreateInfo ToVulkan(const DepthStencilDesc& desc);
vk::PipelineRasterizationStateCreateInfo ToVulkan(const RasterizerDesc& desc);
vk::PipelineColorBlendAttachmentState ToVulkan(const BlendRenderTarget& desc);

TextureAspectFlags FromVulkan(vk::ImageAspectFlags aspct);
vk::IndexType ToVulkan(IndexType indexType);

VkImageCreateInfo ToVulkan(const TextureDesc& desc);

vk::ShaderStageFlags ToVulkan(ShaderStage stage);

vk::AccessFlags ToVulkan(AccessFlags flags);
vk::PipelineStageFlags ToVulkan(PipelineStageFlags flags);

vk::AccessFlags2 ToVulkan2(AccessFlags flags);
vk::PipelineStageFlags2 ToVulkan2(PipelineStageFlags flags);

inline vk::Viewport ToVulkan(const Viewport& viewport) {
    vk::Viewport res{};
    res.x = viewport.x;
    res.y = viewport.y;
    res.width = viewport.width;
    res.height = viewport.height;
    res.minDepth = viewport.minDepth;
    res.maxDepth = viewport.maxDepth;
    return res;
}

inline vk::Extent2D ToVulkan(const Extent2D& ext) {
    return vk::Extent2D{ ext.width, ext.height };
}

inline vk::Rect2D ToVulkan(const Rect2D& ext) {
    return vk::Rect2D{ vk::Offset2D{ ext.offset.x, ext.offset.y }, vk::Extent2D{ ext.extent.width, ext.extent.height } };
}

inline vk::Extent3D ToVulkan(const Extent3D& ext) {
    return vk::Extent3D{ ext.width, ext.height, ext.depth };
}

#define VK_EXPECT(val)                                                                                                                                         \
    if (ugine::gfxapi::vulkan::Failed(val)) {                                                                                                                  \
        return ugine::gfxapi::vulkan::ResultError(val);                                                                                                        \
    }

} // namespace ugine::gfxapi