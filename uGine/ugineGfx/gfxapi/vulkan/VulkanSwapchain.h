#pragma once

#include <ugine/Memory.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <gfxapi/Swapchain.h>
#include <gfxapi/vulkan/Vulkan.h>

namespace ugine::gfxapi {

class VulkanDevice;

class VulkanSwapchain final : public Swapchain {
public:
    static UniquePtr<VulkanSwapchain> Create(VulkanDevice& device, const SwapchainDesc& desc, vk::UniqueSurfaceKHR surface);

    ~VulkanSwapchain();

    bool Present() override;
    void Reset() override;

    Format GetFormat() const override { return format_; }
    Extent2D GetExtent() const override { return Extent2D{ swapchainExtent_.width, swapchainExtent_.height }; }
    TextureHandle GetTexture(u32 index) override { return *textures_[index]; }
    SemaphoreHandle GetWaitSemaphore() override { return *imageAvailableSemaphores_[currentFrame_]; }
    SemaphoreHandle GetSignalSemaphore() override { return *renderFinishedSemaphores_[currentFrame_]; }
    FenceHandle GetFence() override { return *inFlightFences_[currentFrame_]; }
    u32 GetIndex() const override { return frameIndex_; }
    u32 GetCount() const override { return static_cast<u32>(swapchainImages_.size()); }

private:
    explicit VulkanSwapchain(VulkanDevice& device);

    void AcquireNextImage();
    void CreateSwapchain(const SwapchainDesc& desc, vk::UniqueSurfaceKHR surface);
    void CreateSyncs();

    VulkanDevice& device_;
    SwapchainDesc desc_;

    // Swapchain.
    Format format_{};

    vk::Format swapchainImageFormat_;
    vk::Extent2D swapchainExtent_;

    vk::UniqueSurfaceKHR surface_;
    vk::UniqueSwapchainKHR swapchain_;

    // TODO:
    std::vector<vk::Image> swapchainImages_;
    Vector<TextureHandleUnique> textures_;

    // Syncs.
    // TODO: Use vk::Semaphore & vk::Fence.
    Vector<SemaphoreHandleUnique> imageAvailableSemaphores_;
    Vector<SemaphoreHandleUnique> renderFinishedSemaphores_;
    Vector<FenceHandleUnique> inFlightFences_;
    Vector<FenceHandle> imagesInFlight_;

    u32 currentFrame_{};
    u32 frameIndex_{};
};

} // namespace ugine::gfxapi