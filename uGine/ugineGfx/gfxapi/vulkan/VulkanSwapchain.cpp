#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "VulkanInstance.h"

#include <gfxapi/Error.h>
#include <gfxapi/vulkan/Vulkan.h>

#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <format>

namespace ugine::gfxapi {

std::pair<vk::Format, Format> GetSwapchainFormat(const SwapChainCapabilities& swapchainSupport, const SwapchainDesc& desc, vk::ColorSpaceKHR colorSpace) {
    const auto surfaceFormatDesc{ ToVulkan(desc.format) };

    vk::Format surfaceFormat{};
    if (desc.format == Format::Unknown) {
        // If format == Format::Unknown then try to find usable format.
        const auto it{ std::find_if(
            swapchainSupport.formats.begin(), swapchainSupport.formats.end(), [&colorSpace](const auto& format) { return format.colorSpace == colorSpace; }) };

        if (it == swapchainSupport.formats.end()) {
            UGINE_THROW(GfxError, "NotSupported");
        }

        surfaceFormat = it->format;
    } else {
        // Use format from client.
        const auto isFormatSupported{ [&] {
            const auto it{ std::find_if(
                swapchainSupport.formats.begin(), swapchainSupport.formats.end(), [fmt = surfaceFormatDesc](auto format) { return format.format == fmt; }) };
            return it != swapchainSupport.formats.end();
        }() };

        if (!isFormatSupported) {
            UGINE_THROW(GfxError, "NotSupported");
        }

        surfaceFormat = surfaceFormatDesc;
    }

    const auto gfxFormat{ FromVulkan(surfaceFormat) };

    return std::make_pair(surfaceFormat, gfxFormat);
}

vk::Extent2D GetSwapchainExtent(const SwapChainCapabilities& swapchainSupport, const SwapchainDesc& desc) {
    if (desc.width != 0 || desc.height != 0) {
        const auto widthInRange{ desc.width >= swapchainSupport.capabilities.minImageExtent.width
            && desc.width <= swapchainSupport.capabilities.maxImageExtent.width };
        const auto heightInRange{ desc.height >= swapchainSupport.capabilities.minImageExtent.height
            && desc.height <= swapchainSupport.capabilities.maxImageExtent.height };

        if (!widthInRange || !heightInRange) {
            UGINE_THROW(GfxError, "NotSupported");
        }

        return vk::Extent2D{ desc.width, desc.height };
    } else {
        return swapchainSupport.capabilities.currentExtent;
    }
}

UniquePtr<VulkanSwapchain> VulkanSwapchain::Create(VulkanDevice& device, const SwapchainDesc& desc, vk::UniqueSurfaceKHR surface) {
    UniquePtr<VulkanSwapchain> swapchain{ device.Allocator(), UGINE_NEW(device.Allocator(), VulkanSwapchain, device) };
    swapchain->CreateSwapchain(desc, std::move(surface));
    swapchain->CreateSyncs();

    swapchain->AcquireNextImage();

    return std::move(swapchain);
}

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device)
    : device_{ device } //, swapchainImages_{ device.Allocator() }
    , textures_{ device.Allocator() }
    , imageAvailableSemaphores_{ device.Allocator() }
    , renderFinishedSemaphores_{ device.Allocator() }
    , inFlightFences_{ device.Allocator() }
    , imagesInFlight_{ device.Allocator() } {
}

void VulkanSwapchain::CreateSwapchain(const SwapchainDesc& desc, vk::UniqueSurfaceKHR surface) {
    desc_ = desc;
    surface_ = std::move(surface);

    currentFrame_ = 0;
    frameIndex_ = 0;

    const auto swapchainSupport{ QuerySwapChainCapabilities(device_.GetPhysicalDevice(), *surface_) };

    const auto surfaceColorSpace{ vk::ColorSpaceKHR::eSrgbNonlinear };

    // Validate present mode.
    const auto presentMode{ desc.discard ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo };
    const auto isPresentModeSupported{ [&] {
        const auto it{ std::find_if(
            swapchainSupport.presentModes.begin(), swapchainSupport.presentModes.end(), [presentMode](auto mode) { return mode == presentMode; }) };
        return it != swapchainSupport.presentModes.end();
    }() };

    if (!isPresentModeSupported) {
        UGINE_THROW(GfxError, "NotSupported");
    }

    // Validate buffer count.
    if (desc.minBufferCount < swapchainSupport.capabilities.minImageCount
        || (swapchainSupport.capabilities.maxImageCount > 0 && desc.minBufferCount > swapchainSupport.capabilities.maxImageCount)) {
        UGINE_THROW(GfxError, "NotSupported");
    }

    // Validate extent.
    const auto extent{ GetSwapchainExtent(swapchainSupport, desc) };

    // Validate format.
    const auto [vkFormat, gfxFormat] = GetSwapchainFormat(swapchainSupport, desc, surfaceColorSpace);

    const auto graphicsQueue{ device_.GetQueues().graphics };
    const auto presentQueue{ device_.GetQueues().surfacePresent };

    // Create Swapchain.
    format_ = gfxFormat;
    swapchainExtent_ = extent;
    swapchainImageFormat_ = vkFormat;

    // Create swapchain.
    vk::SwapchainCreateInfoKHR swapChainCI{
        .surface = *surface_,
        .minImageCount = desc.minBufferCount,
        .imageFormat = vkFormat,
        .imageColorSpace = surfaceColorSpace,
        .imageExtent = swapchainExtent_,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    };

    const u32 queueFamilyIndices[] = { graphicsQueue, presentQueue };
    if (graphicsQueue != presentQueue) {
        swapChainCI.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCI.queueFamilyIndexCount = 2;
        swapChainCI.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCI.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCI.queueFamilyIndexCount = 1;
        swapChainCI.pQueueFamilyIndices = &presentQueue;
    }

    swapChainCI.preTransform = swapchainSupport.capabilities.currentTransform;
    swapChainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainCI.presentMode = presentMode;
    swapChainCI.clipped = VK_TRUE;

    // Obtain swapchain textures.
    swapchain_ = device_.GetDevice().createSwapchainKHRUnique(swapChainCI);
    swapchainImages_ = device_.GetDevice().getSwapchainImagesKHR(*swapchain_);

    for (u32 index{}; const auto& image : swapchainImages_) {
        TextureDesc textureDesc{
            .name = std::format("Swapchain#{}", index),
            .extent = Extent2D{ swapchainExtent_.width, swapchainExtent_.height },
            .format = gfxFormat,
            .usage = TextureUsageFlags::RenderTarget,
        };

        textures_.EmplaceBack(std::move(device_.CreateTextureHandleFromNativePtrUnique(reinterpret_cast<void*>(VkImage{ image }), textureDesc, false)));
    }
}

VulkanSwapchain::~VulkanSwapchain() {
    auto device{ device_.GetDevice() };

    device_.WaitIdle();

    for (auto& fence : inFlightFences_) {
        auto inFlightFence{ device_.GetFence(*fence) };
        (void)device.waitForFences(inFlightFence, VK_TRUE, UINT64_MAX);
    }
}

void VulkanSwapchain::CreateSyncs() {
    imageAvailableSemaphores_.Clear();
    renderFinishedSemaphores_.Clear();
    inFlightFences_.Clear();

    const auto swapImageCount{ swapchainImages_.size() };

    imageAvailableSemaphores_.Resize(swapImageCount);
    renderFinishedSemaphores_.Resize(swapImageCount);
    inFlightFences_.Resize(swapImageCount);

    for (u32 i{}; i < swapImageCount; ++i) {
        imageAvailableSemaphores_[i] = device_.CreateSemaphoreUnique(vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores_[i] = device_.CreateSemaphoreUnique(vk::SemaphoreCreateInfo{});
        inFlightFences_[i] = device_.CreateFenceUnique(vk::FenceCreateInfo{ /*.flags = vk::FenceCreateFlagBits::eSignaled*/ });
    }

    imagesInFlight_.Resize(u32(swapchainImages_.size()));
}

void VulkanSwapchain::AcquireNextImage() {
    PROFILE_EVENT_N("Acquire");

    auto vkDevice{ device_.GetDevice() };

    //{
    //    auto inFlightFence{ device_.GetFence(*inFlightFences_[currentFrame_]) };
    //    auto result{ device_.GetDevice().waitForFences(inFlightFence, VK_TRUE, UINT64_MAX) };
    //    device_.GetDevice().resetFences(inFlightFence);
    //}

    auto imageAvailable{ device_.GetSemaphore(*imageAvailableSemaphores_[currentFrame_]) };
    const auto acquire{ vkDevice.acquireNextImageKHR(*swapchain_, UINT64_MAX, imageAvailable, {}) };

    frameIndex_ = acquire.value;

    if (imagesInFlight_[frameIndex_]) {
        PROFILE_EVENT_N("Wait fence");

        auto fence{ device_.GetFence(imagesInFlight_[frameIndex_]) };
        auto result{ vkDevice.waitForFences(fence, VK_TRUE, UINT64_MAX) };
        vkDevice.resetFences(fence);
    }

    imagesInFlight_[frameIndex_] = *inFlightFences_[currentFrame_];
}

bool VulkanSwapchain::Present() {
    PROFILE_EVENT_N("Present");

    auto renderFinished{ device_.GetSemaphore(*renderFinishedSemaphores_[currentFrame_]) };

    try {
        auto result{ device_.GetPresentQueue().presentKHR(vk::PresentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinished,
            .swapchainCount = 1,
            .pSwapchains = &(*swapchain_),
            .pImageIndices = &frameIndex_,
            .pResults = nullptr,
        }) };

        currentFrame_ = (currentFrame_ + 1) % u32(swapchainImages_.size());
        if (result != vk::Result::eSuccess) {
            return false;
        }
    } catch (...) {
        return false;
    }

    PROFILE_GPU_PRESENT(*swapchain_);
    AcquireNextImage();

    device_.FinalizeFrame(currentFrame_);

    return true;
}

void VulkanSwapchain::Reset() {
    device_.WaitIdle();

    textures_.Clear();
    swapchainImages_.clear();
    swapchain_ = {};
    surface_ = {};

    for (auto& i : imagesInFlight_) {
        i = {};
    }

    // TODO:
    vk::UniqueSurfaceKHR surface{};
#ifdef WIN32
    surface = device_.Instance().VkInstance().createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR{
        .flags = {},
        .hinstance = (HINSTANCE)desc_.win32.hInstance,
        .hwnd = (HWND)desc_.win32.hWnd,
    });
#else
    UGINE_THROW("Platform not supported (yet)");
#endif

    CreateSwapchain(desc_, std::move(surface));
    CreateSyncs();

    AcquireNextImage();
}

} // namespace ugine::gfxapi