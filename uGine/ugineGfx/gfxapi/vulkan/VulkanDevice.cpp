#include "VulkanDevice.h"
#include "VulkanBindlessPool.h"
#include "VulkanInitializers.h"
#include "VulkanInstance.h"
#include "VulkanSwapchain.h"

#include <gfxapi/Error.h>
#include <gfxapi/Swapchain.h>
#include <gfxapi/spirv/SpirvParser.h>

#include <ugine/Align.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <set>

#ifdef UGINE_VK_TRACE_ALLOCATIONS
#define TRACE_ALLOCATION(memory)                                                                                                                               \
    {                                                                                                                                                          \
        ugine::StackTrace trace;                                                                                                                               \
        trace.Capture();                                                                                                                                       \
        traceAllocations_.insert(std::make_pair(u64(memory), trace.Frames()));                                                                                 \
    }

#define TRACE_DEALLOCATION(memory) traceAllocations_.erase(u64(memory));

#else // UGINE_VK_TRACE_ALLOCATIONS
#define TRACE_ALLOCATION(memory)
#define TRACE_DEALLOCATION(memory)
#endif // UGINE_VK_TRACE_ALLOCATIONS

#define VK_LOAD(NAME) reinterpret_cast<PFN_##NAME>(vkGetDeviceProcAddr(device_, #NAME))

namespace {
PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT_Impl;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT_Impl;
PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT_Impl;
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR_Impl;
PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2_Impl;

#ifdef WIN32
PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR_Impl;
#endif

constexpr u32 GetVulkanApiVersion() {
#if VMA_VULKAN_VERSION == 1003000
    return VK_API_VERSION_1_3;
#elif VMA_VULKAN_VERSION == 1002000
    return VK_API_VERSION_1_2;
#elif VMA_VULKAN_VERSION == 1001000
    return VK_API_VERSION_1_1;
#elif VMA_VULKAN_VERSION == 1000000
    return VK_API_VERSION_1_0;
#else
#error Invalid VMA_VULKAN_VERSION.
    return UINT32_MAX;
#endif
}

} // namespace

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2(VkCommandBuffer commandBuffer, const VkDependencyInfo* pDependencyInfo) {
    vkCmdPipelineBarrier2_Impl(commandBuffer, pDependencyInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
    vkCmdBeginDebugUtilsLabelEXT_Impl(commandBuffer, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) {
    vkCmdEndDebugUtilsLabelEXT_Impl(commandBuffer);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* ext) {
    return vkSetDebugUtilsObjectNameEXT_Impl(device, ext);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, u32 set,
    u32 descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) {
    vkCmdPushDescriptorSetKHR_Impl(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

#ifdef WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) {
    return vkGetMemoryWin32HandleKHR_Impl(device, pGetWin32HandleInfo, pHandle);
}
#endif

namespace ugine::gfxapi {

UniquePtr<Device> Device::Create(const DeviceCreateInfo& info, IAllocator& allocator) {
    return MakeUnique<VulkanDevice>(allocator, info, allocator);
}

const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_16BIT_STORAGE_EXTENSION_NAME,

#ifdef UGINE_VK_PUSH_DESCRIPTORS
    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
#endif

    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,

    // Bindless
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,

    VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME,
};

VulkanDevice::VulkanDevice(const DeviceCreateInfo& info, IAllocator& allocator)
    : allocator_{ allocator }
    , storage_{ MakeUnique<VulkanStorage>(allocator) }
    , dsLayoutCache_{ *this }
    , perFrameCommands_{ allocator }
    , perFrameGraveyard_{ allocator } {
    instance_ = MakeUnique<VulkanInstance>(allocator, info);

    vk::UniqueSurfaceKHR surface;
#ifdef WIN32
    surface = instance_->VkInstance().createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR{
        .flags = {},
        .hinstance = (HINSTANCE)info.win32.hInstance,
        .hwnd = (HWND)info.win32.hWnd,
    });
#else
    UGINE_THROW(GfxError, "Platform not supported (yet)");
#endif

    auto deviceDesciptor{ instance_->FindFirstSuitableDevice(*surface, Span<const char*>(info.vulkan.deviceExtensions, info.vulkan.deviceExtensionsCount)) };
    if (!deviceDesciptor) {
        UGINE_THROW(GfxError, "Failed to create device.");
    }

    // Physical device.
    physicalDevice_ = deviceDesciptor->device;
    UGINE_ASSERT(physicalDevice_);

    properties_ = physicalDevice_.getProperties();

    const auto extensionProperties{ physicalDevice_.enumerateDeviceExtensionProperties() };
    for (const auto& ext : extensionProperties) {
        if (strcmp(ext.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
            UGINE_DEBUG("Debug markers available.");
        }
    }

    // Query features.
    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    vk::PhysicalDeviceFeatures2 features{};
    features.pNext = &indexingFeatures;
    physicalDevice_.getFeatures2(&features);
    supportBindless_ = indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray;

    // Query queues.
    queueFamilies_ = GetDeviceQueues(physicalDevice_, *surface);

    // Device.
    const std::set<u32> uniqueQueues = {
        queueFamilies_.graphics,
        queueFamilies_.compute,
        queueFamilies_.transfer,
        queueFamilies_.surfacePresent,
    };

    const auto queuePriority{ 1.0f };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (auto queueFamily : uniqueQueues) {
        queueCreateInfos.push_back({
            .flags = {},
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        });
    }

    const auto availableFeatures{ physicalDevice_.getFeatures() };
    const auto availableFeatures2{ physicalDevice_.getFeatures2() };

    if (!device_) {
        // Extensions.
        std::vector<const char*> deviceExtensions{ DEVICE_EXTENSIONS };
        for (u32 i{}; i < info.vulkan.deviceExtensionsCount; ++i) {
            deviceExtensions.push_back(info.vulkan.deviceExtensions[i]);
        }

        // Debug.
        if (info.validationLayers) {
            CheckDebug(deviceExtensions);
        }

        // Features.
        vk::PhysicalDeviceVulkan13Features deviceFeatures13{};
        deviceFeatures13.synchronization2 = VK_TRUE;
        deviceFeatures13.dynamicRendering = VK_TRUE;
        deviceFeatures13.shaderDemoteToHelperInvocation = VK_TRUE;

        if (supportBindless_) {
            indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
            indexingFeatures.runtimeDescriptorArray = VK_TRUE;
            // TODO:
            //indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

            deviceFeatures13.pNext = &indexingFeatures;
        }

        // Device.
        vk::PhysicalDeviceFeatures deviceFeatures{};
        if (availableFeatures.samplerAnisotropy) {
            deviceFeatures.samplerAnisotropy = VK_TRUE;
        }
        deviceFeatures.fillModeNonSolid = VK_TRUE;
        deviceFeatures.geometryShader = VK_TRUE;

        vk::DeviceCreateInfo deviceCI{
            .pNext = &deviceFeatures13,
            .queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &deviceFeatures,
        };

        device_ = physicalDevice_.createDevice(deviceCI);
        destroyDevice_ = true;
    }

    device_.getQueue(queueFamilies_.graphics, 0, &graphicsQueue_);
    device_.getQueue(queueFamilies_.compute, 0, &computeQueue_);
    device_.getQueue(queueFamilies_.surfacePresent, 0, &presentQueue_);

    asyncTransfer_ = queueFamilies_.graphics != queueFamilies_.transfer;
    if (asyncTransfer_) {
        device_.getQueue(queueFamilies_.transfer, 0, &transferQueue_);
    }

    availableSampleCount_ = GetMaxUsableSampleCount(physicalDevice_);

    maxCommandList_ = info.vulkan.maxCommandBuffers;

    InitAllocator();

    if (info.validationLayers) { // TODO: validationLayers vs debug (RenderDoc etc.)
        debug_ = true;
        InitDebug();
    }

    InitPlatform();
    InitSwapchain(info, std::move(surface));
    InitCommandBuffers();
    InitProfiling();
    InitExtensions();
    InitBindings();
    InitBindless();

    const std::vector<Format> depthFormatCandidates{
        Format::D32_Float,
        Format::D24_Unorm_S8_Uint,
        Format::D16_Unorm_S8_Uint,
        Format::D16_Unorm,
    };

    const std::vector<Format> depthStencilFormatCandidates{
        Format::D32_Float_S8_Uint,
        Format::D24_Unorm_S8_Uint,
        Format::D16_Unorm_S8_Uint,
    };

    depthFormat_ = FindSupportedFormat(physicalDevice_, depthFormatCandidates, vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage);

    depthStencilFormat_ = FindSupportedFormat(physicalDevice_, depthStencilFormatCandidates, vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage);

    emptyDsLayout_ = device_.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo{
        .flags = {},
        .bindingCount = 0,
        .pBindings = nullptr,
    });
}

VulkanDevice::~VulkanDevice() {
    destroying_ = true;

#ifdef UGINE_PROFILE_TRACY
    TracyVkDestroy(ugine::profile::TracyVkContext);
#endif

    WaitIdle();

    swapchain_ = nullptr;

    imguiDescriptorPool_ = {};
    profilerCommandBuffer_ = {};

    gfxCommandPool_ = {};
    transferCommandPool_ = {};
    perFrameCommands_.Clear();

    // Deferred destroys.
    for (u32 i{}; i < perFrameGraveyard_.Size(); ++i) {
        FinalizeFrame(i);
    }
    perFrameGraveyard_.Clear();
    dsLayoutCache_.Clear();
    emptyDsLayout_ = {};

    bindingsDsPool_ = nullptr;

    {
        // TODO: Destroy storage_->first, since it may need the allocator.
        // storage_->Clear();
        storage_ = nullptr;
    }

    DestroyBindless();

#ifdef UGINE_VK_TRACE_ALLOCATIONS
    if (!traceAllocations_.empty()) {
        UGINE_DEBUG("Dangling {} allocations:", traceAllocations_.size());
        for (const auto& [memory, trace] : traceAllocations_) {
            UGINE_ERROR("Allocation 0x{:x}:", u64(memory));
            for (const auto& frame : trace) {
                UGINE_ERROR("\t\t[0x{:x}] {} ({}:{})", frame.address, frame.symbol.Data(), frame.filename.Data(), frame.line);
            }
            UGINE_ERROR("--------------------");
        }
    }
#endif // UGINE_VK_TRACE_ALLOCATIONS

    vmaDestroyAllocator(vkAllocator_);

    if (destroyDevice_) {
        device_.destroy();
    }
}

void VulkanDevice::Fill(ImGui_ImplVulkan_InitInfo& info) {
    if (!imguiDescriptorPool_) {
        std::vector<vk::DescriptorPoolSize> poolSizes = {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 },
        };
        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = u32(1000 * poolSizes.size());
        poolInfo.poolSizeCount = u32(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        imguiDescriptorPool_ = device_.createDescriptorPoolUnique(poolInfo);
    }

    info.Instance = instance_->VkInstance();
    info.PhysicalDevice = physicalDevice_;
    info.Device = device_;
    info.QueueFamily = queueFamilies_.graphics;
    info.Queue = graphicsQueue_;
    info.PipelineCache = VK_NULL_HANDLE;
    info.DescriptorPool = *imguiDescriptorPool_;
    info.Subpass = 0;
    info.MinImageCount = swapchain_->GetCount();
    info.ImageCount = swapchain_->GetCount();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.Allocator = nullptr;
}

void* VulkanDevice::GetNativeRenderPass(RenderPassHandle handle) {
    UGINE_ASSERT(handle);
    auto vkRenderpass{ storage_->GetRenderPass(handle) };
    UGINE_ASSERT(vkRenderpass);
    return *vkRenderpass->vkRenderpass;
}

void VulkanDevice::InitProfiling() {
    ::VkDevice dev{ device_ };
    ::VkPhysicalDevice phys{ physicalDevice_ };
    ::VkQueue queue{ graphicsQueue_ };

#ifdef UGINE_PROFILE_TRACY
    profilerCommandBuffer_ = std::move(device_.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
        .commandPool = *gfxCommandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    })[0]);
    ugine::profile::TracyVkContext = TracyVkContext(phys, dev, queue, VkCommandBuffer{ *profilerCommandBuffer_ });
#else
    PROFILE_GPU_INIT(dev, phys, queue, queueFamilies_.graphics);
#endif
}

void VulkanDevice::InitExtensions() {
#ifdef UGINE_VK_PUSH_DESCRIPTORS
    vkCmdPushDescriptorSetKHR_Impl = VK_LOAD(vkCmdPushDescriptorSetKHR);
#endif
}

void VulkanDevice::InitBindings() {
    // TODO:
    static const std::vector<vk::DescriptorPoolSize> PoolSizes = {
        { vk::DescriptorType::eUniformBuffer, 8 },
        { vk::DescriptorType::eCombinedImageSampler, 16 },
        { vk::DescriptorType::eStorageBuffer, 8 },
        { vk::DescriptorType::eStorageImage, 8 },
    };

    static const u32 Size{ 256 };

    bindingsDsPool_ = MakeUnique<DescriptorSetPool>(allocator_, *this, PoolSizes, Size, true);
}

void VulkanDevice::InitSwapchain(const DeviceCreateInfo& info, vk::UniqueSurfaceKHR surface) {
    gfxapi::SwapchainDesc desc{
        .minBufferCount = 3,
        .vsync = { .enabled = false, },
        .win32 = {
            .hInstance = info.win32.hInstance,
            .hWnd = info.win32.hWnd,
        },
        .windowed = true, // TODO:
        .discard = true, // TODO:
    };

    swapchain_ = VulkanSwapchain::Create(*this, desc, std::move(surface));
}

void VulkanDevice::InitBindless() {
    // TODO:
    const u32 BINDLESS_IMAGES{ 100000 };
    const u32 BINDLESS_SAMPLERS{ 128 };
    const u32 BINDLESS_BUFFERS{ 100000 };

    bindlessImages_ = MakeUnique<VulkanBindlessPool>(allocator_, *this, vk::DescriptorType::eSampledImage, BINDLESS_IMAGES);
    bindlessSamplers_ = MakeUnique<VulkanBindlessPool>(allocator_, *this, vk::DescriptorType::eSampler, BINDLESS_SAMPLERS);
    //bindlessBuffers_ = MakeUnique<VulkanBindlessPool>(allocator_, *this, vk::DescriptorType::eUniformBuffer, BINDLESS_BUFFERS);
}

void VulkanDevice::DestroyBindless() {
    bindlessImages_ = nullptr;
    bindlessSamplers_ = nullptr;
    //bindlessBuffers_ = nullptr;
}

Swapchain* VulkanDevice::GetSwapchain() {
    return swapchain_.Get();
}

RenderPassHandle VulkanDevice::CreateRenderPass(const RenderPassDesc& desc) {
    std::array<vk::AttachmentDescription, MaxColorAttachments + 1> attachments;
    std::array<vk::AttachmentReference, MaxColorAttachments> colorReference;
    for (u32 i{}; i < desc.colorAttachmentCount; ++i) {
        attachments[i] = ToVulkan(desc.colorAttachments[i]);

        colorReference[i].attachment = i;
        colorReference[i].layout = vk::ImageLayout::eColorAttachmentOptimal;
    }

    vk::AttachmentReference depthReference{};
    if (desc.depthAttachment) {
        attachments[desc.colorAttachmentCount] = ToVulkan(*desc.depthAttachment);

        depthReference.attachment = desc.colorAttachmentCount;
        depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }

    vk::SubpassDescription subpassDescription{
        .flags = {},
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = desc.colorAttachmentCount,
        .pColorAttachments = colorReference.data(),
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = desc.depthAttachment ? &depthReference : nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    std::array<vk::SubpassDependency, 2> dependencies;

    vk::RenderPassCreateInfo renderPassCreateInfo{
        .attachmentCount = desc.colorAttachmentCount + (desc.depthAttachment.has_value() ? 1 : 0),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = 0,
        .pDependencies = dependencies.data(),
    };

    if (desc.inputDependency) {
        auto& dependency{ dependencies[renderPassCreateInfo.dependencyCount++] };

        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = ToVulkan(desc.inputDependency->srcStageMask);
        dependency.dstStageMask = ToVulkan(desc.inputDependency->dstStageMask);
        dependency.srcAccessMask = ToVulkan(desc.inputDependency->srcAccessFlags);
        dependency.dstAccessMask = ToVulkan(desc.inputDependency->dstAccessFlags);
        dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;
    }

    if (desc.outputDependency) {
        auto& dependency{ dependencies[renderPassCreateInfo.dependencyCount++] };
        dependency.srcSubpass = 0;
        dependency.dstSubpass = VK_SUBPASS_EXTERNAL;

        dependency.srcStageMask = ToVulkan(desc.inputDependency->srcStageMask);
        dependency.dstStageMask = ToVulkan(desc.inputDependency->dstStageMask);
        dependency.srcAccessMask = ToVulkan(desc.inputDependency->srcAccessFlags);
        dependency.dstAccessMask = ToVulkan(desc.inputDependency->dstAccessFlags);
        dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;
    }

    VulkanRenderPass vkRenderPass{
        .vkRenderpass = device_.createRenderPassUnique(renderPassCreateInfo),
        .desc = desc,
    };

    if (desc.name) {
        SetDebugName(*vkRenderPass.vkRenderpass, desc.name);
    }

    return storage_->EmplaceRenderPass(std::move(vkRenderPass));
}

void VulkanDevice::DestroyRenderPass(RenderPassHandle handle) {
    perFrameGraveyard_[ActiveFrame()].renderPasses.push_back(handle);
}

FramebufferHandle VulkanDevice::CreateFramebuffer(const FramebufferDesc& desc) {
    UGINE_ASSERT(desc.colorAttachmentCount <= MaxColorAttachments);

    u32 viewCounter{};
    std::array<vk::ImageView, MaxColorAttachments + 1> vkViews;

    for (u32 i{}; i < desc.colorAttachmentCount; ++i) {
        auto colorView{ storage_->GetTexture(desc.colorAttachments[i]) };
        UGINE_ASSERT(colorView);
        UGINE_ASSERT(colorView->rtv.view);

        vkViews[viewCounter++] = colorView->rtv.view;
    }

    auto depthView{ storage_->GetTexture(desc.depth) };

    if (depthView) {
        UGINE_ASSERT(depthView->rtv.view);

        vkViews[viewCounter++] = depthView->rtv.view;
    }

    auto renderpass{ storage_->GetRenderPass(desc.renderPass) };
    UGINE_ASSERT(renderpass);

    vk::FramebufferCreateInfo fbCI{
        .renderPass = *renderpass->vkRenderpass,
        .attachmentCount = viewCounter,
        .pAttachments = vkViews.data(),
        .width = desc.width,
        .height = desc.height,
        .layers = desc.layers,
    };

    VulkanFramebuffer fb{
        .vkFramebuffer = device_.createFramebufferUnique(fbCI),
        .vkRenderPass = *renderpass->vkRenderpass,
        .width = desc.width,
        .height = desc.height,
        .layers = desc.layers,

#ifdef UGINE_VK_TRACE_SYNC
        .desc = desc,
#endif
    };

    if (!desc.name.empty()) {
        SetDebugName(*fb.vkFramebuffer, desc.name);
    }

    return storage_->EmplaceFramebuffer(std::move(fb));
}

void VulkanDevice::DestroyFramebuffer(FramebufferHandle handle) {
    perFrameGraveyard_[ActiveFrame()].framebuffers.push_back(handle);
}

ParsedShaderInfo VulkanDevice::ParseShader(const ShaderParsedData& parsedData, u32 pushDescriptorDataset) {
    ParsedShaderInfo parsed{};

    u32 maxDataset{};
    for (const auto& ds : parsedData.descriptorSets) {
        maxDataset = std::max(ds.set + 1, maxDataset);
    }

    parsed.dsLayouts = Vector<vk::DescriptorSetLayout>(maxDataset, *emptyDsLayout_, allocator_);

    for (const auto& ds : parsedData.descriptorSets) {
        parsed.bindingMask.datasets |= 1 << ds.set;
        parsed.bindingMask.bindings[ds.set] = 0;

        std::array<vk::DescriptorSetLayoutBinding, MaxBindings> bindings;
        for (u32 index{}; const auto& dsBindings : ds.bindings) {
            parsed.bindingMask.bindings[ds.set] |= 1 << dsBindings.binding;

            bindings[index++] = vk::DescriptorSetLayoutBinding{
                .binding = dsBindings.binding,
                .descriptorType = dsBindings.type,
                .descriptorCount = dsBindings.count,
                .stageFlags = dsBindings.stages,
            };
        }

        // TODO: Push descriptor only for last dataset.
        if (ds.bindings.size() > 0 && ds.bindings[0].count == 0) {
            // TODO: Bindless detector.

            switch (ds.bindings[0].type) {
            case vk::DescriptorType::eSampledImage:
                parsed.dsLayouts[ds.set] = bindlessImages_->Layout();
                parsed.bindlessImagesSet = ds.set;
                break;
            case vk::DescriptorType::eSampler:
                parsed.dsLayouts[ds.set] = bindlessSamplers_->Layout();
                parsed.bindlessSamplersSet = ds.set;
                break;
            //case vk::DescriptorType::eUniformBuffer:
            //    parsed.dsLayouts[ds.set] = bindlessBuffers_->Layout();
            //    parsed.bindlessBuffersSet = ds.set;
            //    break;
            default: UGINE_ASSERT(false && "Invald bindless"); break;
            }

        } else if (useDsLayoutCache_) {
            parsed.dsLayouts[ds.set] = dsLayoutCache_.GetOrCreateDescriptorLayout(bindings.data(), u32(ds.bindings.size()), pushDescriptorDataset == ds.set);
        } else {
            vk::DescriptorSetLayoutCreateFlags dsLayoutFlags{};
#ifdef UGINE_VK_PUSH_DESCRIPTORS
            if (pushDescriptorDataset == ds.set) {
                dsLayoutFlags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR;
            }
#endif

            auto layout{ device_.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
                .flags = dsLayoutFlags,
                .bindingCount = static_cast<u32>(ds.bindings.size()),
                .pBindings = bindings.data(),
            }) };

            parsed.dsLayouts[ds.set] = layout;
            parsed.dsLayoutDelete.PushBack(layout);
        }
    }

    std::vector<vk::PushConstantRange> pushConstants;
    for (const auto& ps : parsedData.pushConstants) {
        pushConstants.push_back(vk::PushConstantRange{
            .stageFlags = ps.stage,
            .offset = ps.offset,
            .size = ps.size,
        });
    }

    parsed.pipelineLayout = device_.createPipelineLayout(vk::PipelineLayoutCreateInfo{
        .setLayoutCount = u32(parsed.dsLayouts.Size()),
        .pSetLayouts = parsed.dsLayouts.Data(),
        .pushConstantRangeCount = u32(parsedData.pushConstants.size()),
        .pPushConstantRanges = pushConstants.data(),
    });

    return parsed;
}

GraphicsPipelineHandle VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    UGINE_ASSERT(desc.renderPass);
    const auto vkRenderpass{ storage_->GetRenderPass(desc.renderPass) };

    // Pipeline.
    // TODO: No multisampling for now.
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE,
    };

    std::vector<vk::VertexInputAttributeDescription> vertexAttributes(desc.inputAssembly.vertexAttributesCount);
    for (size_t i{}; i < desc.inputAssembly.vertexAttributesCount; ++i) {
        vertexAttributes[i] = ToVulkan(desc.inputAssembly.vertexAttributes[i]);
    }

    std::array<vk::VertexInputBindingDescription, MaxVertexBindings> vertexBindings{
        vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = desc.inputAssembly.vertexBindings[0].dataStride,
            .inputRate = desc.inputAssembly.vertexBindings[0].slot == InputSlotClass::PerVertex ? vk::VertexInputRate::eVertex : vk::VertexInputRate::eInstance,
        },
        vk::VertexInputBindingDescription{
            .binding = 1,
            .stride = desc.inputAssembly.vertexBindings[1].dataStride,
            .inputRate = desc.inputAssembly.vertexBindings[1].slot == InputSlotClass::PerVertex ? vk::VertexInputRate::eVertex : vk::VertexInputRate::eInstance,
        },
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = desc.inputAssembly.vertexBindingsCount,
        .pVertexBindingDescriptions = vertexBindings.data(),
        .vertexAttributeDescriptionCount = desc.inputAssembly.vertexAttributesCount,
        .pVertexAttributeDescriptions = vertexAttributes.data(),
    };

    const auto inputAssembly{ ToVulkan(desc.inputAssembly) };
    const auto rasterizer{ ToVulkan(desc.rasterizerState) };
    const auto depthStencil{ ToVulkan(desc.depthStencilState) };

    std::array<vk::PipelineColorBlendAttachmentState, MaxColorAttachments> colorBlendAttachments;
    for (u32 i{}; i < vkRenderpass->desc.colorAttachmentCount; ++i) {
        colorBlendAttachments[i] = ToVulkan(desc.blendState.rtv[i]);
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = vkRenderpass->desc.colorAttachmentCount,
        .pAttachments = colorBlendAttachments.data(),
    };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    if (desc.dynamicStates & DynamicPipelineStates::StencilReference) {
        dynamicStates.push_back(vk::DynamicState::eStencilReference);
    }

    if (desc.dynamicStates & DynamicPipelineStates::StencilWriteMask) {
        dynamicStates.push_back(vk::DynamicState::eStencilWriteMask);
    }

    if (desc.dynamicStates & DynamicPipelineStates::StencilCompareMask) {
        dynamicStates.push_back(vk::DynamicState::eStencilCompareMask);
    }

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    vk::Rect2D scissor{ 0, 0 };
    vk::Viewport viewport{ 0, 0 };

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    SpirvParser parser;

    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    auto addStage = [&](vk::ShaderStageFlagBits stage, const std::optional<CompiledShader>& shader) {
        if (shader) {
            auto module{ device_.createShaderModule(vk::ShaderModuleCreateInfo{
                .codeSize = shader->size,
                .pCode = reinterpret_cast<const u32*>(shader->data),
            }) };

            parser.Parse(shader->data, shader->size, stage);

            stages.push_back({
                .stage = stage,
                .module = module,
                .pName = shader->entryPoint,
            });
        }
    };

    addStage(vk::ShaderStageFlagBits::eVertex, desc.vertexShader);
    addStage(vk::ShaderStageFlagBits::eTessellationControl, desc.hullShader);
    addStage(vk::ShaderStageFlagBits::eTessellationEvaluation, desc.domainShader);
    addStage(vk::ShaderStageFlagBits::eGeometry, desc.geometryShader);
    addStage(vk::ShaderStageFlagBits::eFragment, desc.fragmentShader);

    // Layouts.
    auto parsed{ ParseShader(parser.ParsedData(), desc.pushDescriptorDataset) };

    vk::GraphicsPipelineCreateInfo gpCI{
        .stageCount = static_cast<u32>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = parsed.pipelineLayout,
        .renderPass = *vkRenderpass->vkRenderpass,
    };

    VulkanPipeline pipeline{
        .bindPoint = vk::PipelineBindPoint::eGraphics,
        .pipeline = device_.createGraphicsPipeline({}, gpCI).value,
        .layout = parsed.pipelineLayout,
        .descriptorSetLayouts = std::move(parsed.dsLayouts),
        .descriptorSetDelete = std::move(parsed.dsLayoutDelete),
        .bindingMask = parsed.bindingMask,
        .pushDescriptorDataset = desc.pushDescriptorDataset,
        .bindlessImagesDataset = parsed.bindlessImagesSet,
        .bindlessSamplerDataset = parsed.bindlessSamplersSet,
        .bindlessBuffersDataset = parsed.bindlessBuffersSet,
    };

    if (!desc.name.empty()) {
        SetDebugName(parsed.pipelineLayout, desc.name);
        SetDebugName(pipeline.pipeline, desc.name);

        for (auto& layouts : pipeline.descriptorSetDelete) {
            SetDebugName(layouts, desc.name);
        }
    } else {
        SetDebugName(parsed.pipelineLayout, "GfxPipelineLAyout");
        SetDebugName(pipeline.pipeline, "GfxPipeline");

        for (auto& layouts : pipeline.descriptorSetDelete) {
            SetDebugName(layouts, "GfxPipelineDSLayout");
        }
    }

    for (auto& stage : stages) {
        device_.destroyShaderModule(stage.module);
    }

    return storage_->EmplaceGraphicsPipeline(std::move(pipeline));
}

void VulkanDevice::DestroyGraphicsPipeline(GraphicsPipelineHandle handle) {
    perFrameGraveyard_[ActiveFrame()].graphicsPipelines.push_back(handle);
}

ComputePipelineHandle VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc) {
    SpirvParser parser{};
    parser.Parse(desc.computeShader.data, desc.computeShader.size, vk::ShaderStageFlagBits::eCompute);

    auto parsed{ ParseShader(parser.ParsedData(), desc.pushDescriptorDataset) };

    // Pipeline.
    auto module{ device_.createShaderModuleUnique(vk::ShaderModuleCreateInfo{
        .codeSize = desc.computeShader.size,
        .pCode = reinterpret_cast<const u32*>(desc.computeShader.data),
    }) };

    vk::ComputePipelineCreateInfo cpCI {
        .stage = {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = *module,
            .pName = desc.computeShader.entryPoint,
        },
        .layout = parsed.pipelineLayout,
    };

    VulkanPipeline pipeline{
        .bindPoint = vk::PipelineBindPoint::eCompute,
        .pipeline = device_.createComputePipeline({}, cpCI).value,
        .layout = parsed.pipelineLayout,
        .descriptorSetLayouts = parsed.dsLayouts,
        .descriptorSetDelete = parsed.dsLayoutDelete,
        .bindingMask = parsed.bindingMask,
        .pushDescriptorDataset = desc.pushDescriptorDataset,
        .bindlessImagesDataset = parsed.bindlessImagesSet,
        .bindlessSamplerDataset = parsed.bindlessSamplersSet,
        .bindlessBuffersDataset = parsed.bindlessBuffersSet,
    };

    if (!desc.name.empty()) {
        SetDebugName(parsed.pipelineLayout, desc.name);
        SetDebugName(pipeline.pipeline, desc.name);

        for (auto& layouts : pipeline.descriptorSetDelete) {
            SetDebugName(layouts, desc.name);
        }
    } else {
        SetDebugName(parsed.pipelineLayout, "ComputePipelineLayout");
        SetDebugName(pipeline.pipeline, "ComputePipeline");

        for (auto& layouts : pipeline.descriptorSetDelete) {
            SetDebugName(layouts, "ComputePipelineDSLayout");
        }
    }

    return storage_->EmplaceComputePipeline(std::move(pipeline));
}

void VulkanDevice::DestroyComputePipeline(ComputePipelineHandle handle) {
    perFrameGraveyard_[ActiveFrame()].computePipelines.push_back(handle);
}

void* VulkanDevice::GetBufferMapped(BufferHandle buffer) {
    UGINE_ASSERT(buffer);

    auto vkBuffer{ storage_->GetBuffer(buffer) };
    UGINE_ASSERT(vkBuffer->mappedData);

    return vkBuffer->mappedData;
}

BufferHandle VulkanDevice::CreateBuffer(const BufferDesc& desc, const void* initialData, size_t initialDataSize) {
    UGINE_ASSERT(desc.size > 0);
    UGINE_ASSERT(initialDataSize <= desc.size);
    const vk::DeviceSize size{ AlignTo(desc.size, 16) };

    vk::MemoryPropertyFlags memory{};
    if (desc.cpuAccess == CpuAccessFlags::None) {
        memory = vk::MemoryPropertyFlagBits::eDeviceLocal;
    } else {
        memory = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    }

    vk::BufferUsageFlags usage{};
    if (desc.flags & BufferFlags::Uniform) {
        usage |= vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
    }

    if (desc.flags & BufferFlags::Storage) {
        usage |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
    }

    if (desc.flags & BufferFlags::Vertex) {
        usage |= vk::BufferUsageFlagBits::eVertexBuffer;
    }

    if (desc.flags & BufferFlags::Index) {
        usage |= vk::BufferUsageFlagBits::eIndexBuffer;
    }

    if (desc.flags & BufferFlags::Indirect) {
        usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
    }

    if (initialData) {
        usage |= vk::BufferUsageFlagBits::eTransferDst;
    }

    auto buffer{ CreateBuffer(size, usage, memory) };

    if (initialData) {
        auto cmd{ StartOneTimeSubmitCommand() };

        auto stagingBuffer{ CopyBufferData(cmd, buffer, initialData, initialDataSize, 0) };
        EndOneTimeSubmitCommand(cmd);

        DestroyBuffer(std::move(stagingBuffer));
    }

    return storage_->EmplaceBuffer(buffer);
}

void VulkanDevice::DestroyBuffer(BufferHandle handle) {
    UGINE_ASSERT(handle);
    UGINE_ASSERT(storage_->GetBuffer(handle));

    perFrameGraveyard_[ActiveFrame()].buffers.push_back(handle);
}

TextureDesc VulkanDevice::GetTextureDesc(TextureHandle texture) {
    UGINE_ASSERT(texture);
    auto vkImage{ storage_->GetTexture(texture) };
    if (!vkImage) {
        UGINE_THROW(GfxError, "InvalidHandle");
    }

    return vkImage->desc;
}

i32 VulkanDevice::GetTextureBindlessIndex(TextureHandle texture) {
    auto vkImage{ storage_->GetTexture(texture) };
    if (!vkImage) {
        return BindlessInvalid;
    }

    if (vkImage->vkAspect & vk::ImageAspectFlagBits::eColor) {
        return vkImage->colorView.bindlessIndex;
    } else if (vkImage->vkAspect & vk::ImageAspectFlagBits::eDepth) {
        return vkImage->depthView.bindlessIndex;
    } else if (vkImage->vkAspect & vk::ImageAspectFlagBits::eStencil) {
        return vkImage->stencilView.bindlessIndex;
    } else {
        return BindlessInvalid;
    }
}

i32 VulkanDevice::GetTextureBindlessIndex(TextureHandle texture, TextureAspectFlags aspect) {
    UGINE_ASSERT(texture);
    auto vkImage{ storage_->GetTexture(texture) };
    if (!vkImage) {
        return BindlessInvalid;
    }

    switch (aspect) {
    case TextureAspectFlags::Color: return vkImage->colorView.bindlessIndex;
    case TextureAspectFlags::Depth: return vkImage->depthView.bindlessIndex;
    case TextureAspectFlags::Stencil: return vkImage->stencilView.bindlessIndex;
    default: return BindlessInvalid;
    }
}

TextureHandle VulkanDevice::CreateTexture(const TextureDesc& desc, TextureLayout initialLayout, ArrayProxy<SubresourceData> initialData) {
    auto imageCI{ ToVulkan(desc) };

    UGINE_ASSERT(imageCI.extent.width > 0);
    UGINE_ASSERT(imageCI.extent.height > 0);
    UGINE_ASSERT(imageCI.extent.depth > 0);

    if (!initialData.empty()) {
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (desc.misc & TextureMiscFlags::Cube) {
        imageCI.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    if (desc.generateMips) {
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    VkImage vkImage{};
    VmaAllocation allocation{};
    VmaAllocationInfo allocInfo{};

    VmaAllocationCreateInfo allocationCI{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    const auto result{ vmaCreateImage(vkAllocator_, &imageCI, &allocationCI, &vkImage, &allocation, &allocInfo) };
    if (result != VK_SUCCESS) {
        UGINE_THROW(GfxError, "Failed to create image");
    }

    UGINE_ASSERT(result == VK_SUCCESS);
    UGINE_ASSERT(vkImage);
    TRACE_ALLOCATION(vkImage);

    VulkanImage texture{
        .vkImage = vk::Image{ vkImage },
        .vkMemory = vk::DeviceMemory{ allocInfo.deviceMemory },
        .allocation = allocation,
        .desc = desc,
        .vkAspect = AspectFromFormat(vk::Format{ imageCI.format }),
    };

    CreateImageViews(texture);

    const auto vkInitLayout{ ToVulkan(initialLayout) };

    if (!initialData.empty()) {
        auto cmd{ StartOneTimeSubmitCommand() };

        auto buffer{ CopyImageData(cmd, texture, initialData, desc.generateMips ? vk::ImageLayout::eTransferDstOptimal : vkInitLayout) };

        if (desc.generateMips) {
            GenerateMipMaps(cmd, texture.vkImage, desc.arrayLayers, desc.mipLevels, desc.extent.width, desc.extent.height, vk::ImageLayout::eTransferDstOptimal,
                vkInitLayout);
        }

        EndOneTimeSubmitCommand(cmd);
        DestroyBuffer(std::move(buffer));

    } else if (initialLayout != TextureLayout::Undefined) {
        auto cmd{ StartOneTimeSubmitCommand() };

        Transition(cmd, texture, vk::ImageLayout::eUndefined, vkInitLayout, texture.vkAspect);
        EndOneTimeSubmitCommand(cmd);
    }

    if (!desc.name.empty()) {
        SetDebugName(texture.vkImage, desc.name);
    }

    return storage_->EmplaceTexture(std::move(texture));
}

void VulkanDevice::DestroyTexture(TextureHandle handle) {
    UGINE_ASSERT(handle);
    UGINE_ASSERT(storage_->GetTexture(handle)->vkImage);

    perFrameGraveyard_[ActiveFrame()].textures.push_back(handle);
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDesc& desc) {
    if (desc.maxAnisotropy < 1 || desc.maxAnisotropy > 16) {
        UGINE_THROW(GfxError, "InvalidArgument");
    }

    const auto [r, g, b, a] = desc.borderColor;

    vk::BorderColor borderColor{};
    if (r == 1 && g == 1 && b == 1 && a == 1) {
        borderColor = vk::BorderColor::eFloatOpaqueWhite;
    } else if (r == 0 && g == 0 && b == 0) {
        if (a == 0) {
            borderColor = vk::BorderColor::eFloatTransparentBlack;
        } else if (a == 1) {
            borderColor = vk::BorderColor::eFloatOpaqueBlack;
        } else {
            UGINE_THROW(GfxError, "InvalidArgument");
        }
    } else {
        UGINE_THROW(GfxError, "InvalidArgument");
    }

    vk::SamplerCreateInfo samplerCI{
        .magFilter = ToVulkan(desc.magFilter),
        .minFilter = ToVulkan(desc.magFilter),
        .addressModeU = ToVulkan(desc.addressU),
        .addressModeV = ToVulkan(desc.addressV),
        .addressModeW = ToVulkan(desc.addressW),
        .mipLodBias = desc.mipLODBias,
        .anisotropyEnable = properties_.limits.maxSamplerAnisotropy > 1.0f && desc.maxAnisotropy > 1.0f,
        .maxAnisotropy = desc.maxAnisotropy,
        .compareEnable = VK_TRUE, // TODO:
        .compareOp = ToVulkan(desc.comparisonFunc),
        .minLod = desc.minLOD,
        .maxLod = desc.maxLOD,
        .borderColor = borderColor,
    };

    switch (desc.mipmapFilter) {
    case Filter::Linear: samplerCI.mipmapMode = vk::SamplerMipmapMode::eLinear; break;
    case Filter::Nearest: samplerCI.mipmapMode = vk::SamplerMipmapMode::eNearest; break;
    default: UGINE_THROW(GfxError, "InvalidArgument");
    }

    auto sampler{ device_.createSamplerUnique(samplerCI) };
    if (!desc.name.empty()) {
        SetDebugName(*sampler, desc.name);
    }

    const auto bindlessIndex{ bindlessSamplers_->Allocate(vk::DescriptorImageInfo{
        .sampler = *sampler,
    }) };

    return storage_->EmplaceSampler(VulkanSampler{ .sampler = std::move(sampler), .bindlessIndex = bindlessIndex });
}

void VulkanDevice::DestroySampler(SamplerHandle handle) {
    perFrameGraveyard_[ActiveFrame()].samplers.push_back(handle);
}

i32 VulkanDevice::GetSamplerBindlessIndex(SamplerHandle sampler) {
    UGINE_ASSERT(sampler);
    auto vkSampler{ storage_->GetSampler(sampler) };
    return vkSampler ? vkSampler->bindlessIndex : BindlessInvalid;
}

BufferHandle VulkanDevice::CreateVertexBuffer(const void* data, size_t elementSize, size_t elementCount) {
    // TODO: Vertex buffer alignment requirements.
    const vk::DeviceSize size{ AlignTo(elementSize * elementCount, 16) };

    // Create buffer.
    auto vertexBuffer{ CreateBuffer(
        size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal) };

    auto vertexBufferHandle{ storage_->EmplaceBuffer(vertexBuffer) };

    // Copy data through staging buffer.
    auto cmd{ StartOneTimeSubmitCommand() };

    auto buffer{ CopyBufferData(cmd, vertexBuffer, data, elementSize * elementCount) };
    EndOneTimeSubmitCommand(cmd);
    DestroyBuffer(std::move(buffer));

    return vertexBufferHandle;
}

VulkanBuffer VulkanDevice::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
    UGINE_ASSERT(size > 0);

    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = static_cast<VkBufferUsageFlags>(usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocCI{
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    if (properties & vk::MemoryPropertyFlagBits::eDeviceLocal) {
        allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    } else if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        UGINE_ASSERT(false);
    }

    VmaAllocationInfo allocInfo{};

    VulkanBuffer buffer{};
    VkBuffer vkBuffer{};

    const auto result{ vmaCreateBuffer(vkAllocator_, &bufferCI, &allocCI, &vkBuffer, &buffer.allocation, &allocInfo) };
    if (result != VK_SUCCESS) {
        UGINE_THROW(GfxError, "Failed to allocate buffer");
    }

    TRACE_ALLOCATION(vkBuffer);

    buffer.vkBuffer = vk::Buffer{ vkBuffer };
    buffer.vkMemory = vk::DeviceMemory{ allocInfo.deviceMemory };
    buffer.size = size;
    buffer.mappedData = allocInfo.pMappedData;

    return buffer;
}

void VulkanDevice::DestroyBuffer(VulkanBuffer&& buffer) {
    TRACE_DEALLOCATION(VkBuffer{ buffer.vkBuffer });

    vmaDestroyBuffer(vkAllocator_, VkBuffer{ buffer.vkBuffer }, buffer.allocation);
}

BufferHandle VulkanDevice::CreateIndexBuffer(const void* indices, size_t indicesSize) {
    // TODO: Index buffer align requirements.
    const vk::DeviceSize size{ AlignTo(indicesSize, 16) }; // TODO:

    // Create index buffer.
    auto indexBuffer{ CreateBuffer(
        size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal) };

    const auto indexBufferHandle{ storage_->EmplaceBuffer(indexBuffer) };

    // Copy data through staging buffer.
    auto cmd{ StartOneTimeSubmitCommand() };
    auto buffer{ CopyBufferData(cmd, indexBuffer, indices, indicesSize) };
    EndOneTimeSubmitCommand(cmd);

    DestroyBuffer(std::move(buffer));

    return indexBufferHandle;
}

VulkanBuffer VulkanDevice::CopyBufferData(vk::CommandBuffer cmd, VulkanBuffer& dstBuffer, const void* srcData, vk::DeviceSize size, vk::DeviceSize offset) {
    UGINE_ASSERT(dstBuffer.size >= size);

    auto stagingBuffer{ CreateBuffer(
        size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };

    memcpy(stagingBuffer.mappedData, srcData, size);
    cmd.copyBuffer(stagingBuffer.vkBuffer, dstBuffer.vkBuffer,
        vk::BufferCopy{
            .srcOffset = 0,
            .dstOffset = offset,
            .size = size,
        });

    return stagingBuffer;
}

VulkanBuffer VulkanDevice::CopyImageData(vk::CommandBuffer cmd, VulkanImage& dstTexture, ArrayProxy<SubresourceData> initialData, vk::ImageLayout dstLayout) {
    UGINE_ASSERT(!initialData.empty());

    const auto accumulatedSize{ std::accumulate(
        initialData.cbegin(), initialData.cend(), u64(0), [](u64 accum, const auto& data) { return accum + data.size; }) };

    auto stagingBuffer{ CreateBuffer(
        accumulatedSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };

    u64 offset{};
    std::vector<vk::BufferImageCopy> regions(initialData.size());
    for (u32 i{}; i < initialData.size(); ++i) {
        memcpy(reinterpret_cast<char*>(stagingBuffer.mappedData) + offset, initialData[i].data, initialData[i].size);

        // TODO: subresource, mips.
        regions[i].imageExtent = ToVulkan(dstTexture.desc.extent);
        regions[i].bufferOffset = offset;
        regions[i].bufferRowLength = 0;
        regions[i].bufferImageHeight = 0;
        regions[i].imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        regions[i].imageSubresource.mipLevel = 0;
        regions[i].imageSubresource.baseArrayLayer = i;
        regions[i].imageSubresource.layerCount = 1;

        UGINE_ASSERT(i < dstTexture.desc.arrayLayers);

        offset += initialData[i].size;
    }

    Transition(cmd, dstTexture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    cmd.copyBufferToImage(stagingBuffer.vkBuffer, dstTexture.vkImage, vk::ImageLayout::eTransferDstOptimal, regions);

    if (dstLayout != vk::ImageLayout::eTransferDstOptimal) {
        Transition(cmd, dstTexture, vk::ImageLayout::eTransferDstOptimal, dstLayout);
    }

    return stagingBuffer;
}

void VulkanDevice::GenerateMipMaps(
    vk::CommandBuffer cmd, vk::Image image, u32 layers, u32 mipLevels, u32 width, u32 height, vk::ImageLayout initLayout, vk::ImageLayout finalLayout) {
    UGINE_TRACE("Generating mip levels...");

    // Check if image format supports linear blitting
    //vk::FormatProperties formatProperties;
    //vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    //if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    //    throw std::runtime_error("texture image format does not support linear blitting!");
    //}

    vk::ImageMemoryBarrier barrier{};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layers;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth{ static_cast<i32>(width) };
    i32 mipHeight{ static_cast<i32>(height) };

    for (u32 i{ 1 }; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = initLayout;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layers;
        blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.dstOffsets[1] = vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layers;

        cmd.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = finalLayout;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

        if (mipWidth > 1) {
            mipWidth >>= 1;
        }

        if (mipHeight > 1) {
            mipHeight >>= 1;
        }
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = initLayout;
    barrier.newLayout = finalLayout;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);
}

void VulkanDevice::CreateImageViews(VulkanImage& texture) {
    auto createView = [this](VulkanImage& texture, vk::ImageAspectFlags aspect, bool bindless = true) {
        vk::ImageViewType viewType{ vk::ImageViewType::e2D };
        if (texture.desc.misc & TextureMiscFlags::Cube) {
            viewType = vk::ImageViewType::eCube;
        } else if (texture.desc.extent.depth > 1) {
            viewType = vk::ImageViewType::e3D;
        }

        auto imageView {device_.createImageView(vk::ImageViewCreateInfo{
            .flags = {}, 
            .image = texture.vkImage,
            .viewType = viewType,
            .format = ToVulkan(texture.desc.format),
            .subresourceRange = {
                .aspectMask = aspect,
                .baseMipLevel = 0,
                .levelCount = texture.desc.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = texture.desc.arrayLayers,
            },
        }) };

        auto bindlessIndex{ BindlessInvalid };
        if (bindless) {
            vk::DescriptorImageInfo imageInfo{
                .imageView = imageView,
                .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
            };

            bindlessIndex = bindlessImages_->Allocate(imageInfo);
        }

        return VulkanImage::ImageView{
            .view = imageView,
            .bindlessIndex = bindlessIndex,
        };
    };

    if (texture.desc.usage & (TextureUsageFlags::Sampled | TextureUsageFlags::Storage)) {
        if (texture.vkAspect & vk::ImageAspectFlagBits::eColor) {
            texture.colorView = createView(texture, vk::ImageAspectFlagBits::eColor);
            if (!texture.desc.name.empty()) {
                SetDebugName(texture.colorView.view, std::format("{}_COLOR", texture.desc.name));
            }
        }
        if (texture.vkAspect & vk::ImageAspectFlagBits::eDepth) {
            texture.depthView = createView(texture, vk::ImageAspectFlagBits::eDepth);
            if (!texture.desc.name.empty()) {
                SetDebugName(texture.depthView.view, std::format("{}_DEPTH", texture.desc.name));
            }
        }
        if (texture.vkAspect & vk::ImageAspectFlagBits::eStencil) {
            texture.stencilView = createView(texture, vk::ImageAspectFlagBits::eStencil);
            if (!texture.desc.name.empty()) {
                SetDebugName(texture.stencilView.view, std::format("{}_STENCIL", texture.desc.name));
            }
        }
    }

    if (texture.desc.usage & (TextureUsageFlags::RenderTarget | TextureUsageFlags::DepthStencil)) {
        texture.rtv = createView(texture, texture.vkAspect, false);
        if (!texture.desc.name.empty()) {
            SetDebugName(texture.rtv.view, std::format("{}_RTV", texture.desc.name));
        }
    }
}

void VulkanDevice::Transition(
    vk::CommandBuffer cmd, VulkanImage& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspect) const {
    UGINE_ASSERT(newLayout == vk::ImageLayout::eReadOnlyOptimal //
        || newLayout == vk::ImageLayout::eAttachmentOptimal     //
        || newLayout == vk::ImageLayout::eTransferSrcOptimal    //
        || newLayout == vk::ImageLayout::eTransferDstOptimal    //
        || newLayout == vk::ImageLayout::eGeneral               //
    );

    vk::ImageMemoryBarrier2 barrier {
        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstStageMask  =vk::PipelineStageFlagBits2::eAllCommands,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = {},
        .dstQueueFamilyIndex = {},
        .image = image.vkImage,
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = image.desc.mipLevels,
            .baseArrayLayer = 0,
            .layerCount = image.desc.arrayLayers,
        },
    };

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout) {
    case vk::ImageLayout::eUndefined:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        barrier.srcAccessMask = {};
        break;

    case vk::ImageLayout::ePreinitialized:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        barrier.srcAccessMask = vk::AccessFlagBits2::eHostWrite;
        break;

    case vk::ImageLayout::eAttachmentOptimal:
        if (image.vkAspect & vk::ImageAspectFlagBits::eColor) {
            barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        } else {
            barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        }
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
        break;

    case vk::ImageLayout::eTransferDstOptimal:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        break;

    case vk::ImageLayout::eReadOnlyOptimal:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        barrier.srcAccessMask = vk::AccessFlagBits2::eShaderRead;
        break;
    default:
        // Other source layouts aren't handled (yet)
        //UGINE_ASSERT(false && "Implement me!");
        //UGINE_THROW(GfxError, "NotSupported");
        break;
    }

    switch (newLayout) {
    case vk::ImageLayout::eTransferSrcOptimal:
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.dstAccessMask = newLayout == vk::ImageLayout::eTransferSrcOptimal ? vk::AccessFlagBits2::eTransferRead : vk::AccessFlagBits2::eTransferWrite;
        break;
    case vk::ImageLayout::eReadOnlyOptimal: barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead; break;
    case vk::ImageLayout::eGeneral: barrier.dstAccessMask = {}; break;
    case vk::ImageLayout::eAttachmentOptimal:
        if (image.vkAspect & vk::ImageAspectFlagBits::eColor) {
            barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        } else {
            barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        }
        break;
    default: UGINE_ASSERT(false && "Implement me!"); UGINE_THROW(GfxError, "NotSupported");
    }

    cmd.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    });
}

SemaphoreHandle VulkanDevice::CreateSemaphore(const vk::SemaphoreCreateInfo& info) {
    return storage_->EmplaceSemaphore(device_.createSemaphoreUnique(info));
}

void VulkanDevice::DestroySemaphore(SemaphoreHandle handle) {
    perFrameGraveyard_[ActiveFrame()].semaphores.push_back(handle);
}

FenceHandle VulkanDevice::CreateFence(const vk::FenceCreateInfo& info) {
    return storage_->EmplaceFence(device_.createFenceUnique(info));
}

void VulkanDevice::DestroyFence(FenceHandle handle) {
    perFrameGraveyard_[ActiveFrame()].fences.push_back(handle);
}

BindingHandle VulkanDevice::CreateBinding(const BindingDesc& binding) {
    auto pipeline{ binding.bindingPoint == BindingDesc::BindingPoint::Graphics ? storage_->GetGraphicsPipeline(binding.graphicsPipeline)
                                                                               : storage_->GetComputePipeline(binding.computePipeline) };
    UGINE_ASSERT(pipeline);

    VulkanBinding vkBinding{
        .set = binding.set,
        .descriptorSet = bindingsDsPool_->Allocate(pipeline->descriptorSetLayouts[binding.set]),
    };

    std::array<vk::WriteDescriptorSet, MaxBindings> descriptorWrites;
    std::array<vk::DescriptorBufferInfo, MaxBindings> bufferInfos;
    std::array<vk::DescriptorImageInfo, MaxBindings> imageInfos;

    for (u32 i{}; i < binding.bindingsCount; ++i) {
        const auto& b{ binding.bindings[i] };
        auto uniform = [&](const BindingSlot& slot) {
            auto buffer{ storage_->GetBuffer(slot.uniform.buffer) };
            UGINE_ASSERT(buffer);

            bufferInfos[i].buffer = buffer->vkBuffer;
            bufferInfos[i].offset = slot.uniform.offset;
            bufferInfos[i].range = slot.uniform.size;

            descriptorWrites[i] = DescriptorBuffer(vkBinding.descriptorSet, vk::DescriptorType::eUniformBuffer, &bufferInfos[i], slot.binding);
        };

        auto imageSampler = [&](const BindingSlot& slot) {
            auto image{ storage_->GetTexture(slot.imageSampler.texture) };
            UGINE_ASSERT(image);
            auto sampler{ storage_->GetSampler(slot.imageSampler.sampler) };
            UGINE_ASSERT(sampler);

            imageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfos[i].sampler = *sampler->sampler;

            // TODO:
            if (image->vkAspect & vk::ImageAspectFlagBits::eColor) {
                imageInfos[i].imageView = image->colorView.view;
            } else if (image->vkAspect & vk::ImageAspectFlagBits::eDepth) {
                imageInfos[i].imageView = image->depthView.view;
            } else {
                imageInfos[i].imageView = image->stencilView.view;
            }

            descriptorWrites[i] = DescriptorImage(vkBinding.descriptorSet, vk::DescriptorType::eCombinedImageSampler, &imageInfos[i], slot.binding);
        };

        switch (b.type) {
            using enum BindingSlot::Type;

        case Uniform: uniform(b); break;
        case ImageSampler: imageSampler(b); break;
        case Storage: UGINE_ASSERT(false && "Implement me!"); break;
        }
    }

    device_.updateDescriptorSets(binding.bindingsCount, descriptorWrites.data(), 0, nullptr);
    return storage_->EmplaceBinding(vkBinding);
}

vk::Fence VulkanDevice::GetFence(FenceHandle handle) const {
    UGINE_ASSERT(handle);
    return storage_->GetFence(handle);
}

vk::Semaphore VulkanDevice::GetSemaphore(SemaphoreHandle handle) const {
    UGINE_ASSERT(handle);
    return storage_->GetSemaphore(handle);
}

void VulkanDevice::SetDebugName(vk::ObjectType type, u64 handle, std::string_view name) {
    UGINE_ASSERT(handle);
    UGINE_ASSERT(!name.empty());

    if (debugExtension_) {
        device_.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{
            .objectType = type,
            .objectHandle = handle,
            .pObjectName = name.data(),
        });
    }
}

void VulkanDevice::InitCommandBuffers() {
    gfxCommandPool_ = device_.createCommandPoolUnique(vk::CommandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilies_.graphics,
    });

    if (asyncTransfer_) {
        transferCommandPool_ = device_.createCommandPoolUnique(vk::CommandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilies_.transfer,
        });
    }

    perFrameCommands_.Resize(swapchain_->GetCount());
    perFrameGraveyard_.Resize(swapchain_->GetCount());

    for (int frameIndex{}; auto& frame : perFrameCommands_) {
        // Per-frame.
        frame.command.Resize(maxCommandList_);
        for (auto& command : frame.command) {
            // Per-thread.
            command.Initialize(this);
        }

        ++frameIndex;
    }
}

void VulkanDevice::DestroyCommands() {
    WaitIdle();

    perFrameCommands_.Clear();
}

void VulkanDevice::WaitIdle() {
    if (graphicsQueue_) {
        graphicsQueue_.waitIdle();
    }

    if (presentQueue_) {
        presentQueue_.waitIdle();
    }

    if (device_) {
        device_.waitIdle();
    }

    cmdStart_ = 0;
    cmdIndex_ = 0;
}

void VulkanDevice::SetDebugName(TextureHandle texture, const char* name) {
    UGINE_ASSERT(texture);
    auto vkTexture{ storage_->GetTexture(texture) };
    UGINE_ASSERT(vkTexture);
    UGINE_ASSERT(name);

    if (vkTexture->vkImage) {
        SetDebugName(vkTexture->vkImage, name);
    }
    if (vkTexture->rtv.view) {
        SetDebugName(vkTexture->rtv.view, name);
    }
    if (vkTexture->colorView.view) {
        SetDebugName(vkTexture->colorView.view, name);
    }
    if (vkTexture->depthView.view) {
        SetDebugName(vkTexture->depthView.view, name);
    }
    if (vkTexture->stencilView.view) {
        SetDebugName(vkTexture->stencilView.view, name);
    }
}

void VulkanDevice::ResetCommands() {
    cmdIndex_ = 0;
    cmdStart_ = 0;
}

CommandList* VulkanDevice::BeginCommandList(CommandList::Type type, bool gfxQueue) {
    auto cmd{ cmdIndex_.fetch_add(1) };

    UGINE_ASSERT(cmd < maxCommandList_);

    // Prepare commands.
    auto& command{ FrameCommands().command[cmd] };
    command.Begin(type, gfxQueue);
    return &command;
}

void VulkanDevice::SubmitCommandLists() {
    auto waitSemaphore{ storage_->GetSemaphore(swapchain_->GetWaitSemaphore()) };
    auto signalSemaphore{ storage_->GetSemaphore(swapchain_->GetSignalSemaphore()) };
    auto fence{ storage_->GetFence(swapchain_->GetFence()) };

    const vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    SubmitCommandLists(&waitSemaphore, waitStages, 1, &signalSemaphore, 1, fence);
    ResetCommands();
}

void VulkanDevice::SubmitCommandLists(vk::Semaphore* waitSemaphores, const vk::PipelineStageFlags* waitStages, u32 waitSemaphoresCount,
    vk::Semaphore* signalSemaphores, u32 signalSemaphoresCount, vk::Fence fence) {
    PROFILE_EVENT_N("Submit commands");

    auto start{ cmdStart_.load() };
    auto count{ cmdIndex_.load() };
    cmdStart_ = count;

    // TODO:
    const auto MAX_COMMANDLIST_COUNT{ 32 };

    vk::CommandBuffer gfxCommands[MAX_COMMANDLIST_COUNT];
    vk::CommandBuffer computeCommands[MAX_COMMANDLIST_COUNT];

    // TODO: vk::SubmitInfo
    vk::SubmitInfo gfxSubmit{
        .waitSemaphoreCount = waitSemaphoresCount,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .pCommandBuffers = gfxCommands,
        .signalSemaphoreCount = signalSemaphoresCount,
        .pSignalSemaphores = signalSemaphores,
    };

    vk::SubmitInfo computeSubmit{
        .waitSemaphoreCount = waitSemaphoresCount,
        .pWaitSemaphores = waitSemaphores,
        .pCommandBuffers = computeCommands,
        .signalSemaphoreCount = signalSemaphoresCount,
        .pSignalSemaphores = signalSemaphores,
    };

    for (u32 i{ start }; i < count; ++i) {
        auto& command{ FrameCommands().command[i] };
        PROFILE_GPU_COLLECT(command.VkCommandBuffer());
        command.End();

        switch (command.CommandType()) {
            using enum CommandList::Type;
        case Graphics: gfxCommands[gfxSubmit.commandBufferCount++] = command.VkCommandBuffer(); break;
        case Compute: computeCommands[computeSubmit.commandBufferCount++] = command.VkCommandBuffer(); break;
        }
    }

    if (gfxSubmit.commandBufferCount && computeSubmit.commandBufferCount) {
        UGINE_ASSERT(false); // TODO: Makes no sence, at least from the semaphore / fence point of view.
        UGINE_WARN("Compute command and gfx commands in single command list.");
    }

    if (computeSubmit.commandBufferCount) {
        computeQueue_.submit(computeSubmit, fence);
    }

    if (gfxSubmit.commandBufferCount) {
        graphicsQueue_.submit(gfxSubmit, fence);
    }
}

vk::CommandBuffer VulkanDevice::StartOneTimeSubmitCommand() {
    auto cmdBuffer{ device_.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = *gfxCommandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    })[0] };

    cmdBuffer.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    });

    return cmdBuffer;
}

void VulkanDevice::EndOneTimeSubmitCommand(vk::CommandBuffer cmd) {
    cmd.end();

    graphicsQueue_.submit(
        vk::SubmitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        },
        nullptr);

    // TODO: Improve.
    graphicsQueue_.waitIdle();
    device_.freeCommandBuffers(*gfxCommandPool_, cmd);
}

VkResult VulkanDevice::AllocateBuffer(const VkBufferCreateInfo* bufferCI, const VmaAllocationCreateInfo* allocCI, VkBuffer* vkBuffer,
    VmaAllocation* vmaAllocation, VmaAllocationInfo* allocInfo) {
    auto result{ vmaCreateBuffer(vkAllocator_, bufferCI, allocCI, vkBuffer, vmaAllocation, allocInfo) };

    if (result == VK_SUCCESS) {
        TRACE_ALLOCATION(*vkBuffer);
    }

    return result;
}

u32 VulkanDevice::FindMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const {
    const auto memProperties{ physicalDevice_.getMemoryProperties() };
    for (u32 i{}; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    UGINE_ASSERT(false && "Memory type not found.");
    return 0;
}

void VulkanDevice::CheckDebug(std::vector<const char*>& deviceExtensions) {
    const auto instanceExtensions{ vk::enumerateInstanceExtensionProperties() };

    for (auto& ext : instanceExtensions) {
        if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            debugExtension_ = true;
            deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            break;
        }
    }
}

void VulkanDevice::InitAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{
        .physicalDevice = VkPhysicalDevice{ physicalDevice_ },
        .device = VkDevice{ device_ },
        .instance = VkInstance{ instance_->VkInstance() },
        .vulkanApiVersion = instance_->VkVersion(),
    };

    const auto res{ vmaCreateAllocator(&allocatorInfo, &vkAllocator_) };
    if (res != VK_SUCCESS) {
        UGINE_THROW(GfxError, "Failed to create allocator");
    }
}

void VulkanDevice::InitDebug() {
    if (debugExtension_) {
        vkCmdBeginDebugUtilsLabelEXT_Impl = VK_LOAD(vkCmdBeginDebugUtilsLabelEXT);
        vkCmdEndDebugUtilsLabelEXT_Impl = VK_LOAD(vkCmdEndDebugUtilsLabelEXT);
        vkSetDebugUtilsObjectNameEXT_Impl = VK_LOAD(vkSetDebugUtilsObjectNameEXT);
    }
}

void VulkanDevice::InitPlatform() {
#ifdef WIN32
    vkGetMemoryWin32HandleKHR_Impl = VK_LOAD(vkGetMemoryWin32HandleKHR);
#endif

    vkCmdPipelineBarrier2_Impl = VK_LOAD(vkCmdPipelineBarrier2);
}

TextureHandle VulkanDevice::CreateTextureHandleFromNativePtr(void* nativeApiHandle, const TextureDesc& desc, bool takeOwnership) {
    vk::Image image{ reinterpret_cast<VkImage>(nativeApiHandle) };

    VulkanImage texture{
        .vkImage = image,
        .vkMemory = {},
        .allocation = {},
        .desc = desc,
        .vkAspect = AspectFromFormat(ToVulkan(desc.format)),
        .ownership = takeOwnership,
    };

    CreateImageViews(texture);

    return storage_->EmplaceTexture(std::move(texture));
}

TextureAspectFlags VulkanDevice::GetTextureAspect(TextureHandle texture) const {
    auto vkTexture{ storage_->GetTexture(texture) };
    UGINE_ASSERT(texture);

    return FromVulkan(vkTexture->vkAspect);
}

void VulkanDevice::FinalizeFrame(u32 index) {
    // Since we are reusing commands make sure that previous instance finished.

    // TODO:
    //commandList_[index].Wait();

    auto& graveyard{ perFrameGraveyard_[index] };

    for (auto handle : graveyard.buffers) {
        DestroyBufferImmediately(handle);
    }
    for (auto handle : graveyard.samplers) {
        DestroySamplerImmediately(handle);
    }
    for (auto handle : graveyard.graphicsPipelines) {
        DestroyGraphicsPipelineImmediately(handle);
    }
    for (auto handle : graveyard.computePipelines) {
        DestroyComputePipelineImmediately(handle);
    }
    for (auto handle : graveyard.textures) {
        DestroyTextureImmediately(handle);
    }
    for (auto handle : graveyard.renderPasses) {
        DestroyRenderPassImmediately(handle);
    }
    for (auto handle : graveyard.framebuffers) {
        DestroyFramebufferImmediately(handle);
    }
    for (auto handle : graveyard.fences) {
        DestroyFenceImmediately(handle);
    }
    for (auto handle : graveyard.semaphores) {
        DestroySemaphoreImmediately(handle);
    }
    for (auto handle : graveyard.bindings) {
        DestroyBindingImmediately(handle);
    }
    for (auto handle : graveyard.queryPools) {
        DestroyQueryPoolImmediately(handle);
    }

    graveyard.Clear();
}

void VulkanDevice::DestroyRenderPassImmediately(RenderPassHandle handle) {
    storage_->DestroyRenderPass(handle);
}

void VulkanDevice::DestroyFramebufferImmediately(FramebufferHandle handle) {
    storage_->DestroyFramebuffer(handle);
}

void VulkanDevice::DestroyGraphicsPipelineImmediately(GraphicsPipelineHandle handle) {
    auto pipeline{ storage_->GetGraphicsPipeline(handle) };
    device_.destroyPipelineLayout(pipeline->layout);
    device_.destroyPipeline(pipeline->pipeline);

    for (auto& ds : pipeline->descriptorSetDelete) {
        device_.destroyDescriptorSetLayout(ds);
    }

    storage_->DestroyGraphicsPipeline(handle);
}

void VulkanDevice::DestroyComputePipelineImmediately(ComputePipelineHandle handle) {
    auto pipeline{ storage_->GetComputePipeline(handle) };
    device_.destroyPipelineLayout(pipeline->layout);
    device_.destroyPipeline(pipeline->pipeline);

    for (auto& ds : pipeline->descriptorSetDelete) {
        device_.destroyDescriptorSetLayout(ds);
    }

    storage_->DestroyComputePipeline(handle);
}

void VulkanDevice::DestroyBufferImmediately(BufferHandle handle) {
    auto buffer{ storage_->GetBuffer(handle) };
    UGINE_ASSERT(buffer);
    if (buffer) {
        DestroyBuffer(std::move(*buffer));
    }

    storage_->DestroyBuffer(handle);
}

void VulkanDevice::DestroyTextureImmediately(TextureHandle handle) {
    UGINE_ASSERT(handle);
    auto vkTexture{ storage_->GetTexture(handle) };

    UGINE_ASSERT(vkTexture);

    if (vkTexture->ownership) {
        if (vkTexture->allocation) {
            TRACE_DEALLOCATION(VkImage{ vkTexture->vkImage });

            vmaDestroyImage(vkAllocator_, VkImage{ vkTexture->vkImage }, vkTexture->allocation);
        } else {
            vkFreeMemory(VkDevice{ device_ }, VkDeviceMemory{ vkTexture->vkMemory }, nullptr);
            vkDestroyImage(VkDevice{ device_ }, VkImage{ vkTexture->vkImage }, nullptr);
        }
    }

    auto destroyView = [&](VulkanImage::ImageView& view) {
        if (view.view) {
            device_.destroyImageView(view.view);
        }
        if (view.bindlessIndex != BindlessInvalid) {
            bindlessImages_->Free(view.bindlessIndex);
        }
    };

    destroyView(vkTexture->rtv);
    destroyView(vkTexture->colorView);
    destroyView(vkTexture->depthView);
    destroyView(vkTexture->stencilView);

    storage_->DestroyTexture(handle);
}

void VulkanDevice::DestroySamplerImmediately(SamplerHandle handle) {
    auto sampler{ storage_->GetSampler(handle) };
    UGINE_ASSERT(sampler);

    if (sampler->bindlessIndex != BindlessInvalid) {
        bindlessSamplers_->Free(sampler->bindlessIndex);
    }

    storage_->DestroySampler(handle);
}

void VulkanDevice::DestroySemaphoreImmediately(SemaphoreHandle handle) {
    storage_->DestroySemaphore(handle);
}

void VulkanDevice::DestroyFenceImmediately(FenceHandle handle) {
    storage_->DestroyFence(handle);
}

void VulkanDevice::DestroyBindingImmediately(BindingHandle handle) {
    auto binding{ storage_->GetBinding(handle) };
    UGINE_ASSERT(binding);

    bindingsDsPool_->Free(binding->descriptorSet);
}

VulkanDevice::PerFrameCommands& VulkanDevice::FrameCommands() {
    return perFrameCommands_[ActiveFrame()];
}

u32 VulkanDevice::ActiveFrame() const {
    if (destroying_) {
        return 0;
    }

    UGINE_ASSERT(swapchain_);

    return swapchain_->GetIndex();
}

void VulkanDevice::DeferredDestroy(vk::UniqueDescriptorPool pool) {
    if (!destroying_) {
        perFrameGraveyard_[ActiveFrame()].descriptorPools.push_back(std::move(pool));
    }
}

void VulkanDevice::DestroyBinding(BindingHandle binding) {
    // TODO: Binding resuing...

    if (!destroying_) {
        perFrameGraveyard_[ActiveFrame()].bindings.push_back(binding);
    } else {
        DestroyBindingImmediately(binding);
    }
}

vk::DescriptorSet VulkanDevice::BindlessImages() const {
    return bindlessImages_->Descriptor();
}

vk::DescriptorSet VulkanDevice::BindlessSamplers() const {
    return bindlessSamplers_->Descriptor();
}

QueryPoolHandle VulkanDevice::CreateQueryPool(const QueryPoolDesc& desc) {
    return storage_->EmplaceQueryPool(*this, ToVulkan(desc.type), desc.count);
}

void VulkanDevice::DestroyQueryPool(QueryPoolHandle handle) {
    perFrameGraveyard_[ActiveFrame()].queryPools.push_back(handle);
}

void VulkanDevice::DestroyQueryPoolImmediately(QueryPoolHandle handle) {
    storage_->DestroyQueryPool(handle);
}

Span<const u64> VulkanDevice::FetchQueryPoolResults(QueryPoolHandle handle) {
    auto queryPool{ storage_->GetQueryPool(handle) };
    UGINE_ASSERT(queryPool);

    queryPool->FetchResults();
    return queryPool->Results();
}

double VulkanDevice::GetMicroseconds(u64 timestamp) const {
    return double(timestamp * Properties().limits.timestampPeriod) / 1000.0;
}

//vk::DescriptorSet VulkanDevice::BindlessBuffers() const {
//    return bindlessBuffers_->Descriptor();
//}

} // namespace ugine::gfxapi