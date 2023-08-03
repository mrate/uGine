#include <gfxapi/CommandList.h>
#include <gfxapi/Error.h>
#include <gfxapi/Types.h>
#include <gfxapi/vulkan/Vulkan.h>

#include <ugine/Color.h>
#include <ugine/Ugine.h>

#include <array>
#include <map>
#include <set>

namespace ugine::gfxapi {

vk::Format FindSupportedFormat(
    vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        auto props{ physicalDevice.getFormatProperties(format) };

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    UGINE_THROW(GfxError, "Failed to find supported format");
}

Format FindSupportedFormat(vk::PhysicalDevice& physicalDevice, const std::vector<Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (auto format : candidates) {
        auto props{ physicalDevice.getFormatProperties(ToVulkan(format)) };

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    UGINE_THROW(GfxError, "Failed to find supported format");
}

bool CheckDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char*> requiredExtensions) {
    const auto availableExtensions{ device.enumerateDeviceExtensionProperties() };
    std::set<std::string> requiredExtensionsSet{ requiredExtensions.begin(), requiredExtensions.end() };

    for (const auto& extension : availableExtensions) {
        requiredExtensionsSet.erase(extension.extensionName);
    }

    return requiredExtensionsSet.empty();
}

SwapChainCapabilities QuerySwapChainCapabilities(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
    return SwapChainCapabilities{
        .capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface),
        .formats = physicalDevice.getSurfaceFormatsKHR(surface),
        .presentModes = physicalDevice.getSurfacePresentModesKHR(surface),
    };
}

std::optional<DeviceDescriptor> IsDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface, const std::vector<const char*> requiredExtensions) {
    const auto deviceProperties{ device.getProperties() };
    const auto deviceFeatures{ device.getFeatures() };
    const auto queueFamilies{ device.getQueueFamilyProperties() };

    bool hasGfxCmdQueue{};
    bool hasComputeCmdQueue{};
    bool hasTransferCmdQueue{};
    bool supportSurfacePresent{};

    DeviceDescriptor outputDesciptor{};
    outputDesciptor.device = device;
    outputDesciptor.surface = surface;

    int i{};
    for (const auto& family : queueFamilies) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            hasGfxCmdQueue = true;
            outputDesciptor.queueFamilies.graphics = i;
        }

        if ((family.queueFlags & vk::QueueFlagBits::eCompute) && !(family.queueFlags & vk::QueueFlagBits::eGraphics)) {
            hasComputeCmdQueue = true;
            outputDesciptor.queueFamilies.compute = i;
        }

        if ((family.queueFlags & vk::QueueFlagBits::eTransfer) && !(family.queueFlags & vk::QueueFlagBits::eGraphics)
            && !(family.queueFlags & vk::QueueFlagBits::eCompute)) {
            hasTransferCmdQueue = true;
            outputDesciptor.queueFamilies.transfer = i;
        }

        vk::Bool32 presentSupport{};
        const auto result{ device.getSurfaceSupportKHR(i, surface, &presentSupport) };

        if (result == vk::Result::eSuccess && presentSupport) {
            supportSurfacePresent = true;
            outputDesciptor.queueFamilies.surfacePresent = i;
        }

        ++i;

        if (hasGfxCmdQueue && supportSurfacePresent && hasComputeCmdQueue && hasTransferCmdQueue) {
            break;
        }
    }

    if (!hasTransferCmdQueue) {
        outputDesciptor.queueFamilies.transfer = outputDesciptor.queueFamilies.graphics;
    }

    if (!hasComputeCmdQueue) {
        outputDesciptor.queueFamilies.compute = outputDesciptor.queueFamilies.graphics;
    }

    const auto extensionsSupported{ CheckDeviceExtensionSupport(device, requiredExtensions) };

    bool swapchainAdequate{};
    if (extensionsSupported) {
        const auto swapchainSupport{ QuerySwapChainCapabilities(device, surface) };
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }

    if (                              //deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu // Is discrete GPU.
        deviceFeatures.geometryShader // Supports geometry shader.
        && hasGfxCmdQueue             // Has graphics command queue.
        && supportSurfacePresent      // Has surface presentation queue.
        && extensionsSupported        // Supports all required extensions.
        && swapchainAdequate          // Swap chain has all required capabilities.
    ) {
        return outputDesciptor;
    }

    return {};
}

DeviceDescriptor GetFirstSuitableDevice(vk::Instance instance, vk::SurfaceKHR surface, const std::vector<const char*> requiredExtensions) {
    const auto devices{ instance.enumeratePhysicalDevices() };
    if (devices.empty()) {
        UGINE_THROW(GfxError, "NotSupported");
    }

    for (const auto& device : devices) {
        const auto deviceDesc{ IsDeviceSuitable(device, surface, requiredExtensions) };
        if (deviceDesc) {
            return *deviceDesc;
        }
    }

    UGINE_THROW(GfxError, "NotSupported");
}

vk::SampleCountFlagBits GetMaxUsableSampleCount(vk::PhysicalDevice physicalDevice) {
    vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

    const auto counts{ physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts };
    if (counts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }

    return vk::SampleCountFlagBits::e1;
}

QueueFamilies GetDeviceQueues(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
    const auto queueFamilies{ physicalDevice.getQueueFamilyProperties() };

    bool hasGfxCmdQueue{};
    bool hasComputeCmdQueue{};
    bool hasTransferCmdQueue{};
    bool supportSurfacePresent{};

    QueueFamilies queues{};

    int i{};
    for (const auto& family : queueFamilies) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            hasGfxCmdQueue = true;
            queues.graphics = i;
        }

        if ((family.queueFlags & vk::QueueFlagBits::eCompute) && !(family.queueFlags & vk::QueueFlagBits::eGraphics)) {
            hasComputeCmdQueue = true;
            queues.compute = i;
        }

        if ((family.queueFlags & vk::QueueFlagBits::eTransfer) && !(family.queueFlags & vk::QueueFlagBits::eGraphics)
            && !(family.queueFlags & vk::QueueFlagBits::eCompute)) {
            hasTransferCmdQueue = true;
            queues.transfer = i;
        }

        vk::Bool32 presentSupport{};
        const auto result{ physicalDevice.getSurfaceSupportKHR(i, surface, &presentSupport) };

        if (result == vk::Result::eSuccess && presentSupport) {
            supportSurfacePresent = true;
            queues.surfacePresent = i;
        }

        ++i;

        if (hasGfxCmdQueue && supportSurfacePresent && hasComputeCmdQueue && hasTransferCmdQueue) {
            break;
        }
    }

    return queues;
}

Format FromVulkan(vk::Format format) {
    static const std::map<vk::Format, Format> FMT_MAP = {
        { vk::Format::eUndefined, Format::Unknown },
        { vk::Format::eR8Uint, Format::R8_Uint },
        { vk::Format::eR8G8Uint, Format::R8G8_Uint },
        { vk::Format::eR8G8B8A8Uint, Format::R8G8B8A8_Uint },
        { vk::Format::eR8Unorm, Format::R8_Unorm },
        { vk::Format::eR8G8Unorm, Format::R8G8_Unorm },
        { vk::Format::eR8G8B8A8Unorm, Format::R8G8B8A8_Unorm },
        { vk::Format::eR8G8B8A8Srgb, Format::R8G8B8A8_Unorm_Srgb },
        { vk::Format::eR16Uint, Format::R16_Uint },
        { vk::Format::eR16G16Uint, Format::R16G16_Uint },
        { vk::Format::eR16G16B16A16Uint, Format::R16G16B16A16_Uint },
        { vk::Format::eR32Uint, Format::R32_Uint },
        { vk::Format::eR32G32Uint, Format::R32G32_Uint },
        { vk::Format::eR32G32B32Uint, Format::R32G32B32_Uint },
        { vk::Format::eR32G32B32A32Uint, Format::R32G32B32A32_Uint },
        { vk::Format::eR32Sfloat, Format::R32_Float },
        { vk::Format::eR32G32Sfloat, Format::R32G32_Float },
        { vk::Format::eR32G32B32Sfloat, Format::R32G32B32_Float },
        { vk::Format::eR32G32B32A32Sfloat, Format::R32G32B32A32_Float },
        { vk::Format::eB8G8R8A8Unorm, Format::B8G8R8A8_Unorm },
        { vk::Format::eB8G8R8A8Srgb, Format::B8G8R8A8_Unorm_Srgb },
        { vk::Format::eR16G16B16A16Sfloat, Format::R16G16B16A16_Float },
        { vk::Format::eD24UnormS8Uint, Format::D24_Unorm_S8_Uint },
        { vk::Format::eD32Sfloat, Format::D32_Float },
        { vk::Format::eD16UnormS8Uint, Format::D16_Unorm_S8_Uint },
        { vk::Format::eD16Unorm, Format::D16_Unorm },
        { vk::Format::eD32SfloatS8Uint, Format::D32_Float_S8_Uint },

    };

    return FMT_MAP.at(format);
}

vk::Format ToVulkan(Format format) {
    static const std::map<Format, vk::Format> FMT_MAP = {
        { Format::Unknown, vk::Format::eUndefined },
        { Format::R8_Uint, vk::Format::eR8Uint },
        { Format::R8G8_Uint, vk::Format::eR8G8Uint },
        { Format::R8G8B8A8_Uint, vk::Format::eR8G8B8A8Uint },
        { Format::R8_Unorm, vk::Format::eR8Unorm },
        { Format::R8G8_Unorm, vk::Format::eR8G8Unorm },
        { Format::R8G8B8A8_Unorm, vk::Format::eR8G8B8A8Unorm },
        { Format::R8G8B8A8_Unorm_Srgb, vk::Format::eR8G8B8A8Srgb },
        { Format::R16_Uint, vk::Format::eR16Uint },
        { Format::R16G16_Uint, vk::Format::eR16G16Uint },
        { Format::R16G16B16A16_Uint, vk::Format::eR16G16B16A16Uint },
        { Format::R32_Uint, vk::Format::eR32Uint },
        { Format::R32G32_Uint, vk::Format::eR32G32Uint },
        { Format::R32G32B32_Uint, vk::Format::eR32G32B32Uint },
        { Format::R32G32B32A32_Uint, vk::Format::eR32G32B32A32Uint },
        { Format::R32_Float, vk::Format::eR32Sfloat },
        { Format::R32G32_Float, vk::Format::eR32G32Sfloat },
        { Format::R32G32B32_Float, vk::Format::eR32G32B32Sfloat },
        { Format::R32G32B32A32_Float, vk::Format::eR32G32B32A32Sfloat },
        { Format::B8G8R8A8_Unorm, vk::Format::eB8G8R8A8Unorm },
        { Format::B8G8R8A8_Unorm_Srgb, vk::Format::eB8G8R8A8Srgb },
        { Format::R16G16B16A16_Float, vk::Format::eR16G16B16A16Sfloat },
        { Format::D24_Unorm_S8_Uint, vk::Format::eD24UnormS8Uint },
        { Format::D32_Float, vk::Format::eD32Sfloat },
        { Format::D16_Unorm_S8_Uint, vk::Format::eD16UnormS8Uint },
        { Format::D16_Unorm, vk::Format::eD16Unorm },
        { Format::D32_Float_S8_Uint, vk::Format::eD32SfloatS8Uint },
    };

    // TODO: Static assert.
    UGINE_ASSERT(FMT_MAP.size() == size_t(Format::COUNT));

    return FMT_MAP.at(format);
}

vk::ClearValue ToVulkanColorClearValue(const ClearValue& color) {
    vk::ClearValue clearValue{};
    clearValue.color.float32[0] = color.color.r;
    clearValue.color.float32[1] = color.color.g;
    clearValue.color.float32[2] = color.color.b;
    clearValue.color.float32[3] = color.color.a;
    return clearValue;
}

vk::ClearValue ToVulkanDepthStencilClearValue(const ClearValue& color) {
    vk::ClearValue clearValue{};
    clearValue.depthStencil.depth = color.depthStencil.depth;
    clearValue.depthStencil.stencil = color.depthStencil.stencil;
    return clearValue;
}

vk::AttachmentLoadOp ToVulkan(AttachmentLoadOp op) {
    switch (op) {
    case AttachmentLoadOp::Load: return vk::AttachmentLoadOp::eLoad;
    case AttachmentLoadOp::Clear: return vk::AttachmentLoadOp::eClear;
    case AttachmentLoadOp::DontCare: return vk::AttachmentLoadOp::eDontCare;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::AttachmentStoreOp ToVulkan(AttachmentStoreOp op) {
    switch (op) {
    case AttachmentStoreOp::DontCare: return vk::AttachmentStoreOp::eDontCare;
    case AttachmentStoreOp::Store: return vk::AttachmentStoreOp::eStore;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::AttachmentDescription ToVulkan(const RenderPassAttachmentDesc& desc) {
    vk::AttachmentDescription attachment{};
    attachment.format = ToVulkan(desc.format);
    attachment.loadOp = ToVulkan(desc.loadOp);
    attachment.storeOp = ToVulkan(desc.storeOp);
    attachment.stencilLoadOp = ToVulkan(desc.stencilLoadOp);
    attachment.stencilStoreOp = ToVulkan(desc.stencilStoreOp);
    attachment.initialLayout = ToVulkan(desc.initialLayout);
    attachment.finalLayout = ToVulkan(desc.finalLayout);
    return attachment;
}

//vk::ImageViewType ToVulkan(TextureViewType type) {
//    switch (type) {
//    case TextureViewType::Texture2D:
//        return vk::ImageViewType::e2D;
//    case TextureViewType::Texture2DArray:
//        return vk::ImageViewType::e2DArray;
//    case TextureViewType::Texture3D:
//        return vk::ImageViewType::e3D;
//    case TextureViewType::TextureCube:
//        return vk::ImageViewType::eCube;
//    default:
//        UGINE_ASSERT(false);
//        UGINE_THROW(GfxError, "InvalidArgument");
//    }
//}
//

TextureAspectFlags FromVulkan(vk::ImageAspectFlags aspect) {
    TextureAspectFlags result{};

    if (aspect & vk::ImageAspectFlagBits::eColor) {
        result |= TextureAspectFlags::Color;
    }
    if (aspect & vk::ImageAspectFlagBits::eDepth) {
        result |= TextureAspectFlags::Depth;
    }
    if (aspect & vk::ImageAspectFlagBits::eStencil) {
        result |= TextureAspectFlags::Stencil;
    }

    return result;
}

vk::IndexType ToVulkan(IndexType indexType) {
    switch (indexType) {
    case IndexType::Uint16: return vk::IndexType::eUint16;
    case IndexType::Uint32: return vk::IndexType::eUint32;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::CompareOp ToVulkan(ComparisonFunc func) {
    switch (func) {
    case ComparisonFunc::Never: return vk::CompareOp::eNever;
    case ComparisonFunc::Less: return vk::CompareOp::eLess;
    case ComparisonFunc::Equal: return vk::CompareOp::eEqual;
    case ComparisonFunc::LessEqual: return vk::CompareOp::eLessOrEqual;
    case ComparisonFunc::Greater: return vk::CompareOp::eGreater;
    case ComparisonFunc::NotEqual: return vk::CompareOp::eNotEqual;
    case ComparisonFunc::GreaterEqual: return vk::CompareOp::eGreaterOrEqual;
    case ComparisonFunc::Always: return vk::CompareOp::eAlways;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::ImageLayout ToVulkan(TextureLayout layout) {
    switch (layout) {
    case TextureLayout::Undefined: return vk::ImageLayout::eUndefined;
    case TextureLayout::General: return vk::ImageLayout::eGeneral;
    case TextureLayout::Present: return vk::ImageLayout::ePresentSrcKHR;
    case TextureLayout::TransferSrc: return vk::ImageLayout::eTransferSrcOptimal;
    case TextureLayout::TransferDst: return vk::ImageLayout::eTransferDstOptimal;
    case TextureLayout::Attachment: return vk::ImageLayout::eAttachmentOptimal;
    case TextureLayout::ReadOnly: return vk::ImageLayout::eReadOnlyOptimal;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::Filter ToVulkan(Filter filter) {
    switch (filter) {
    case Filter::Linear: return vk::Filter::eLinear;
    case Filter::Nearest: return vk::Filter::eNearest;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::SamplerAddressMode ToVulkan(const TextureAddressMode& mode) {
    switch (mode) {
    case TextureAddressMode::Wrap: return vk::SamplerAddressMode::eRepeat;
    case TextureAddressMode::Mirror: return vk::SamplerAddressMode::eMirroredRepeat;
    case TextureAddressMode::Clamp: return vk::SamplerAddressMode::eClampToEdge;
    case TextureAddressMode::Border: return vk::SamplerAddressMode::eClampToBorder;
    case TextureAddressMode::MirrorOnce: return vk::SamplerAddressMode::eMirrorClampToEdge;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::VertexInputAttributeDescription ToVulkan(const VertexAttribute& desc) {
    return vk::VertexInputAttributeDescription{
        .location = desc.location,
        .binding = desc.group,
        .format = ToVulkan(desc.format),
        .offset = desc.offset,
    };
}

vk::ClearValue ToVulkan(const ClearValue& value) {
    std::array<f32, 4> rgba = { value.color.r, value.color.g, value.color.b, value.color.a };

    vk::ClearValue res{};
    res.color = vk::ClearColorValue(rgba);
    res.depthStencil = vk::ClearDepthStencilValue(value.depthStencil.depth, value.depthStencil.stencil);
    return res;
}

vk::PipelineInputAssemblyStateCreateInfo ToVulkan(const InputAssemblyDesc& desc) {
    vk::PipelineInputAssemblyStateCreateInfo res{};
    switch (desc.primitiveTopology) {
    case PrimitiveTopology::TriangleList: res.topology = vk::PrimitiveTopology::eTriangleList; break;
    case PrimitiveTopology::LineList: res.topology = vk::PrimitiveTopology::eLineList; break;
    case PrimitiveTopology::PointList: res.topology = vk::PrimitiveTopology::ePointList; break;
    case PrimitiveTopology::LineStrip: res.topology = vk::PrimitiveTopology::eLineStrip; break;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "InvalidArgument");
    }

    return res;
}

vk::StencilOp ToVulkan(StencilOp op) {
    switch (op) {
    case StencilOp::Keep: return vk::StencilOp::eKeep;
    case StencilOp::Zero: return vk::StencilOp::eZero;
    case StencilOp::Replace: return vk::StencilOp::eReplace;
    case StencilOp::IncrementAndClamp: return vk::StencilOp::eIncrementAndClamp;
    case StencilOp::DecrementAndClamp: return vk::StencilOp::eDecrementAndClamp;
    case StencilOp::Invert: return vk::StencilOp::eInvert;
    case StencilOp::IncrementAndWrap: return vk::StencilOp::eIncrementAndWrap;
    case StencilOp::DecrementAndWrap: return vk::StencilOp::eIncrementAndWrap;
    default: UGINE_ASSERT(false); UGINE_THROW(GfxError, "Invalid argument");
    }
}

vk::StencilOpState ToVulkan(const StencilOpDesc& desc) {
    vk::StencilOpState res{};

    res.failOp = ToVulkan(desc.failOp);
    res.passOp = ToVulkan(desc.passOp);
    res.compareOp = ToVulkan(desc.compareOp);
    res.depthFailOp = ToVulkan(desc.depthFailOp);
    res.reference = desc.reference;
    res.writeMask = desc.writeMask;
    res.compareMask = desc.compareMask;

    return res;
}

vk::PipelineDepthStencilStateCreateInfo ToVulkan(const DepthStencilDesc& desc) {
    return vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable = desc.depthTestEnable,
        .depthWriteEnable = desc.depthWriteEnable,
        .depthCompareOp = ToVulkan(desc.depthFunc),
        .stencilTestEnable = desc.stencilEnable,
        .front = ToVulkan(desc.frontFace),
        .back = ToVulkan(desc.backFace),
    };
}

vk::CullModeFlagBits ToVulkan(CullMode cullMode) {
    switch (cullMode) {
    case CullMode::Back: return vk::CullModeFlagBits::eBack;
    case CullMode::Front: return vk::CullModeFlagBits::eFront;
    case CullMode::None: return vk::CullModeFlagBits::eNone;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::PolygonMode ToVulkan(FillMode fillMode) {
    switch (fillMode) {
    case FillMode::Fill: return vk::PolygonMode::eFill;
    case FillMode::Line: return vk::PolygonMode::eLine;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::QueryType ToVulkan(QueryType queryType) {
    switch (queryType) {
    case QueryType::Timestamp: return vk::QueryType::eTimestamp;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::PipelineStageFlags2 ToVulkan(PipelineStage stage) {
    switch (stage) {
    case PipelineStage::TopOfPipeline: return vk::PipelineStageFlagBits2::eTopOfPipe;
    case PipelineStage::BottomOfPipeline: return vk::PipelineStageFlagBits2::eBottomOfPipe;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::PipelineRasterizationStateCreateInfo ToVulkan(const RasterizerDesc& desc) {
    return vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable = VK_FALSE,
        .polygonMode = ToVulkan(desc.fillMode),
        .cullMode = ToVulkan(desc.cullMode),
        .frontFace = desc.frontCCW ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise,
        .depthBiasEnable = desc.depthBiasEnable ? VK_TRUE : VK_FALSE,
        .depthBiasConstantFactor = desc.depthBiasConstantFactor,
        .depthBiasClamp = {},
        .depthBiasSlopeFactor = desc.depthBiasSlopeFactor,
        .lineWidth = 1.0f,
    };
}

vk::BlendFactor ToVulkan(Blend blend) {
    switch (blend) {
    case Blend::Zero: return vk::BlendFactor::eZero;
    case Blend::One: return vk::BlendFactor::eOne;
    case Blend::SrcColor: return vk::BlendFactor::eSrcColor;
    case Blend::OneMinusSrcColor: return vk::BlendFactor::eOneMinusSrcColor;
    case Blend::SrcAlpha: return vk::BlendFactor::eSrcAlpha;
    case Blend::OneMinusSrcAlpha: return vk::BlendFactor::eOneMinusSrcAlpha;
    case Blend::DstAlpha: return vk::BlendFactor::eDstAlpha;
    case Blend::OneMinusDstAlpha: return vk::BlendFactor::eOneMinusDstAlpha;
    case Blend::DstColor: return vk::BlendFactor::eDstColor;
    case Blend::OneMinusDstColor: return vk::BlendFactor::eOneMinusDstColor;
    case Blend::SrcAlphaSat: return vk::BlendFactor::eSrcAlphaSaturate;
    //case Blend::BlendFactor:
    //    return vk::BlendFactor::eConstantAlpha;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::BlendOp ToVulkan(BlendOp blendOp) {
    switch (blendOp) {
    case BlendOp::Add: return vk::BlendOp::eAdd;
    case BlendOp::Subtract: return vk::BlendOp::eSubtract;
    case BlendOp::RevSubtract: return vk::BlendOp::eReverseSubtract;
    case BlendOp::Min: return vk::BlendOp::eMin;
    case BlendOp::Max: return vk::BlendOp::eMax;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }
}

vk::PipelineColorBlendAttachmentState ToVulkan(const BlendRenderTarget& desc) {

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = desc.enable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.colorBlendOp = ToVulkan(desc.blendOp);
    colorBlendAttachment.srcColorBlendFactor = ToVulkan(desc.srcBlend);
    colorBlendAttachment.dstColorBlendFactor = ToVulkan(desc.dstBlend);

    colorBlendAttachment.alphaBlendOp = ToVulkan(desc.alphaBlendOp);
    colorBlendAttachment.srcAlphaBlendFactor = ToVulkan(desc.srcAlphaBlend);
    colorBlendAttachment.dstAlphaBlendFactor = ToVulkan(desc.dstAlphaBlend);

    colorBlendAttachment.colorWriteMask
        = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    return colorBlendAttachment;
}

vk::BufferCopy ToVulkan(const BufferCopy& copy) {
    return vk::BufferCopy{
        .srcOffset = copy.srcOffset,
        .dstOffset = copy.dstOffset,
        .size = copy.size,
    };
}

vk::StencilFaceFlags ToVulkan(StencilFaceFlags flags) {
    vk::StencilFaceFlags res{};
    if (flags & StencilFaceFlags::Front) {
        res |= vk::StencilFaceFlagBits::eFront;
    }
    if (flags & StencilFaceFlags::Back) {
        res |= vk::StencilFaceFlagBits::eBack;
    }
    if (flags & StencilFaceFlags::FrontAndBack) {
        res |= vk::StencilFaceFlagBits::eFrontAndBack;
    }
    return res;
}

VkImageCreateInfo ToVulkan(const TextureDesc& desc) {
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.arrayLayers = desc.arrayLayers;
    imageCI.extent = VkExtent3D{ desc.extent.width, desc.extent.height, desc.extent.depth };
    imageCI.format = static_cast<VkFormat>(ToVulkan(desc.format));
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.imageType = desc.extent.depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    imageCI.mipLevels = desc.mipLevels;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT; // TODO:
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;

    imageCI.usage = {};
    if (desc.usage & TextureUsageFlags::RenderTarget) {
        imageCI.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.usage & TextureUsageFlags::DepthStencil) {
        imageCI.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (desc.usage & TextureUsageFlags::Sampled) {
        imageCI.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usage & TextureUsageFlags::Storage) {
        imageCI.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usage & TextureUsageFlags::TransferSrc) {
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc.usage & TextureUsageFlags::TransferDst) {
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    return imageCI;
}

bool FormatHasDepth(vk::Format format) {
    switch (format) {
    case vk::Format::eD16Unorm:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD32SfloatS8Uint: return true;
    default: return false;
    }
}

bool FormatHasStencil(vk::Format format) {
    switch (format) {
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint: return true;
    default: return false;
    }
}

vk::ImageAspectFlags AspectFromFormat(vk::Format format) {
    if (FormatHasDepth(format)) {
        if (FormatHasStencil(format)) {
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        } else {
            return vk::ImageAspectFlagBits::eDepth;
        }
    } else {
        return vk::ImageAspectFlagBits::eColor;
    }
}

vk::ShaderStageFlags ToVulkan(ShaderStage stage) {
    vk::ShaderStageFlags res{};
    res |= stage & ShaderStage::VertexShader ? vk::ShaderStageFlagBits::eVertex : vk::ShaderStageFlagBits{};
    res |= stage & ShaderStage::HullShader ? vk::ShaderStageFlagBits::eTessellationControl : vk::ShaderStageFlagBits{};
    res |= stage & ShaderStage::DomainShader ? vk::ShaderStageFlagBits::eTessellationEvaluation : vk::ShaderStageFlagBits{};
    res |= stage & ShaderStage::GeometryShader ? vk::ShaderStageFlagBits::eGeometry : vk::ShaderStageFlagBits{};
    res |= stage & ShaderStage::FragmentShader ? vk::ShaderStageFlagBits::eFragment : vk::ShaderStageFlagBits{};
    res |= stage & ShaderStage::ComputeShader ? vk::ShaderStageFlagBits::eCompute : vk::ShaderStageFlagBits{};

    return res;
}

vk::AccessFlags ToVulkan(AccessFlags flags) {
    vk::AccessFlags res{};

    res |= (flags & AccessFlags::IndirectCommandRead) ? vk::AccessFlagBits::eIndirectCommandRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::IndexRead) ? vk::AccessFlagBits::eIndexRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::VertexAttributeRead) ? vk::AccessFlagBits::eVertexAttributeRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::UniformRead) ? vk::AccessFlagBits::eUniformRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::InputAttachmentRead) ? vk::AccessFlagBits::eInputAttachmentRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::ShaderRead) ? vk::AccessFlagBits::eShaderRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::ShaderWrite) ? vk::AccessFlagBits::eShaderWrite : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::ColorAttachmentRead) ? vk::AccessFlagBits::eColorAttachmentRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::ColorAttachmentWrite) ? vk::AccessFlagBits::eColorAttachmentWrite : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::DepthStencilAttachmentRead) ? vk::AccessFlagBits::eDepthStencilAttachmentRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::DepthStencilAttachmentWrite) ? vk::AccessFlagBits::eDepthStencilAttachmentWrite : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::TransferRead) ? vk::AccessFlagBits::eTransferRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::TransferWrite) ? vk::AccessFlagBits::eTransferWrite : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::HostRead) ? vk::AccessFlagBits::eHostRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::HostWrite) ? vk::AccessFlagBits::eHostWrite : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::MemoryRead) ? vk::AccessFlagBits::eMemoryRead : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::MemoryWrite) ? vk::AccessFlagBits::eMemoryWrite : vk::AccessFlagBits{};
    res |= (flags & AccessFlags::None) ? vk::AccessFlagBits::eNone : vk::AccessFlagBits{};

    return res;
}

vk::PipelineStageFlags ToVulkan(PipelineStageFlags flags) {
    vk::PipelineStageFlags res{};

    res |= (flags & PipelineStageFlags::TopOfPipe) ? vk::PipelineStageFlagBits::eTopOfPipe : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::DrawIndirect) ? vk::PipelineStageFlagBits::eDrawIndirect : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::VertexInput) ? vk::PipelineStageFlagBits::eVertexInput : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::VertexShader) ? vk::PipelineStageFlagBits::eVertexShader : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::TessellationControlShader) ? vk::PipelineStageFlagBits::eTessellationControlShader : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::TessellationEvaluationShader) ? vk::PipelineStageFlagBits::eTessellationEvaluationShader : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::GeometryShader) ? vk::PipelineStageFlagBits::eGeometryShader : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::FragmentShader) ? vk::PipelineStageFlagBits::eFragmentShader : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::EarlyFragmentTests) ? vk::PipelineStageFlagBits::eEarlyFragmentTests : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::LateFragmentTests) ? vk::PipelineStageFlagBits::eLateFragmentTests : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::ColorAttachmentOutput) ? vk::PipelineStageFlagBits::eColorAttachmentOutput : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::ComputeShader) ? vk::PipelineStageFlagBits::eComputeShader : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::Transfer) ? vk::PipelineStageFlagBits::eTransfer : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::BottomOfPipe) ? vk::PipelineStageFlagBits::eBottomOfPipe : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::Host) ? vk::PipelineStageFlagBits::eHost : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::AllGraphics) ? vk::PipelineStageFlagBits::eAllGraphics : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::AllCommands) ? vk::PipelineStageFlagBits::eAllCommands : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::None) ? vk::PipelineStageFlagBits::eNone : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::TransformFeedback) ? vk::PipelineStageFlagBits::eTransformFeedbackEXT : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::ConditionalRendering) ? vk::PipelineStageFlagBits::eConditionalRenderingEXT : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::AccelerationStructureBuild) ? vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::RayTracingShader) ? vk::PipelineStageFlagBits::eRayTracingShaderKHR : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::FragmentDensityProcess) ? vk::PipelineStageFlagBits::eFragmentDensityProcessEXT : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::FragmentShadingRateAttachment) ? vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR
                                                                       : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::CommandPreprocess) ? vk::PipelineStageFlagBits::eCommandPreprocessNV : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::TaskShader) ? vk::PipelineStageFlagBits::eTaskShaderEXT : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::MeshShader) ? vk::PipelineStageFlagBits::eMeshShaderEXT : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::AccelerationStructureBuild) ? vk::PipelineStageFlagBits::eAccelerationStructureBuildNV : vk::PipelineStageFlagBits{};
    res |= (flags & PipelineStageFlags::RayTracingShader) ? vk::PipelineStageFlagBits::eRayTracingShaderNV : vk::PipelineStageFlagBits{};

    return res;
}

vk::AccessFlags2 ToVulkan2(AccessFlags flags) {
    vk::AccessFlags2 res{};

    res |= (flags & AccessFlags::IndirectCommandRead) ? vk::AccessFlagBits2::eIndirectCommandRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::IndexRead) ? vk::AccessFlagBits2::eIndexRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::VertexAttributeRead) ? vk::AccessFlagBits2::eVertexAttributeRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::UniformRead) ? vk::AccessFlagBits2::eUniformRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::InputAttachmentRead) ? vk::AccessFlagBits2::eInputAttachmentRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::ShaderRead) ? vk::AccessFlagBits2::eShaderRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::ShaderWrite) ? vk::AccessFlagBits2::eShaderWrite : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::ColorAttachmentRead) ? vk::AccessFlagBits2::eColorAttachmentRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::ColorAttachmentWrite) ? vk::AccessFlagBits2::eColorAttachmentWrite : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::DepthStencilAttachmentRead) ? vk::AccessFlagBits2::eDepthStencilAttachmentRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::DepthStencilAttachmentWrite) ? vk::AccessFlagBits2::eDepthStencilAttachmentWrite : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::TransferRead) ? vk::AccessFlagBits2::eTransferRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::TransferWrite) ? vk::AccessFlagBits2::eTransferWrite : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::HostRead) ? vk::AccessFlagBits2::eHostRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::HostWrite) ? vk::AccessFlagBits2::eHostWrite : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::MemoryRead) ? vk::AccessFlagBits2::eMemoryRead : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::MemoryWrite) ? vk::AccessFlagBits2::eMemoryWrite : vk::AccessFlagBits2{};
    res |= (flags & AccessFlags::None) ? vk::AccessFlagBits2::eNone : vk::AccessFlagBits2{};

    return res;
}

vk::PipelineStageFlags2 ToVulkan2(PipelineStageFlags flags) {
    vk::PipelineStageFlags2 res{};

    res |= (flags & PipelineStageFlags::TopOfPipe) ? vk::PipelineStageFlagBits2::eTopOfPipe : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::DrawIndirect) ? vk::PipelineStageFlagBits2::eDrawIndirect : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::VertexInput) ? vk::PipelineStageFlagBits2::eVertexInput : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::VertexShader) ? vk::PipelineStageFlagBits2::eVertexShader : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::TessellationControlShader) ? vk::PipelineStageFlagBits2::eTessellationControlShader : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::TessellationEvaluationShader) ? vk::PipelineStageFlagBits2::eTessellationEvaluationShader
                                                                      : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::GeometryShader) ? vk::PipelineStageFlagBits2::eGeometryShader : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::FragmentShader) ? vk::PipelineStageFlagBits2::eFragmentShader : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::EarlyFragmentTests) ? vk::PipelineStageFlagBits2::eEarlyFragmentTests : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::LateFragmentTests) ? vk::PipelineStageFlagBits2::eLateFragmentTests : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::ColorAttachmentOutput) ? vk::PipelineStageFlagBits2::eColorAttachmentOutput : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::ComputeShader) ? vk::PipelineStageFlagBits2::eComputeShader : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::Transfer) ? vk::PipelineStageFlagBits2::eTransfer : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::BottomOfPipe) ? vk::PipelineStageFlagBits2::eBottomOfPipe : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::Host) ? vk::PipelineStageFlagBits2::eHost : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::AllGraphics) ? vk::PipelineStageFlagBits2::eAllGraphics : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::AllCommands) ? vk::PipelineStageFlagBits2::eAllCommands : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::None) ? vk::PipelineStageFlagBits2::eNone : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::TransformFeedback) ? vk::PipelineStageFlagBits2::eTransformFeedbackEXT : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::ConditionalRendering) ? vk::PipelineStageFlagBits2::eConditionalRenderingEXT : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::AccelerationStructureBuild) ? vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::RayTracingShader) ? vk::PipelineStageFlagBits2::eRayTracingShaderKHR : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::FragmentDensityProcess) ? vk::PipelineStageFlagBits2::eFragmentDensityProcessEXT : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::FragmentShadingRateAttachment) ? vk::PipelineStageFlagBits2::eFragmentShadingRateAttachmentKHR
                                                                       : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::CommandPreprocess) ? vk::PipelineStageFlagBits2::eCommandPreprocessNV : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::TaskShader) ? vk::PipelineStageFlagBits2::eTaskShaderEXT : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::MeshShader) ? vk::PipelineStageFlagBits2::eMeshShaderEXT : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::AccelerationStructureBuild) ? vk::PipelineStageFlagBits2::eAccelerationStructureBuildNV : vk::PipelineStageFlagBits2{};
    res |= (flags & PipelineStageFlags::RayTracingShader) ? vk::PipelineStageFlagBits2::eRayTracingShaderNV : vk::PipelineStageFlagBits2{};

    return res;
}

} // namespace ugine::gfxapi