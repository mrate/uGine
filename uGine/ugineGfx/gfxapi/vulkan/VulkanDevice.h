#pragma once

#include <gfxapi/Device.h>
#include <gfxapi/vulkan/Vulkan.h>

#include <ugine/StackTrace.h>

#include "VulkanCommandList.h"
#include "VulkanDescriptorSetLayoutCache.h"
#include "VulkanDescriptorSetPool.h"
#include "VulkanResourceList.h"
#include "VulkanResources.h"

#include <map>

#ifdef CreateSemaphore
// Thank you, Windows!
#undef CreateSemaphore
#endif

namespace ugine::gfxapi {

struct DeviceCreateInfo;
struct ShaderParsedData;

class VulkanInstance;
class VulkanSwapchain;
class VulkanBindlessPool;

struct ParsedShaderInfo {
    vk::PipelineLayout pipelineLayout;
    Vector<vk::DescriptorSetLayout> dsLayouts;
    Vector<vk::DescriptorSetLayout> dsLayoutDelete;
    VulkanDescriptorBindingMask bindingMask{};
    u32 bindlessImagesSet{ MaxDatasets };
    u32 bindlessSamplersSet{ MaxDatasets };
    u32 bindlessBuffersSet{ MaxDatasets };
};

class VulkanDevice final : public Device {
public:
    VulkanDevice(const DeviceCreateInfo& info, IAllocator& allocator = IAllocator::Default());
    ~VulkanDevice();

    void Fill(ImGui_ImplVulkan_InitInfo& info) override;
    void* GetNativeRenderPass(RenderPassHandle handle) override;

    // Vulkan specific:
    [[nodiscard]] vk::DescriptorSet BindlessImages() const;
    [[nodiscard]] vk::DescriptorSet BindlessSamplers() const;
    //[[nodiscard]] vk::DescriptorSet BindlessBuffers() const;

    VulkanStorage& GetStorage() { return *storage_; }
    u32 FramesInFlight() const { return u32(perFrameCommands_.Size()); }

    VulkanInstance& Instance() { return *instance_; }

    void ResetCommands();

    [[nodiscard]] IAllocator& Allocator() const { return allocator_; }

    [[nodiscard]] vk::CommandBuffer StartOneTimeSubmitCommand();
    [[nodiscard]] void EndOneTimeSubmitCommand(vk::CommandBuffer);

    vk::PhysicalDeviceProperties Properties() const { return properties_; }

    void DeferredDestroy(vk::UniqueDescriptorPool pool);
    bool Debug() const { return debug_; }

    //[[nodiscard]] VmaAllocator GetAllocator() const { return allocator_; }

    VkResult AllocateBuffer(const VkBufferCreateInfo* bufferCI, const VmaAllocationCreateInfo* allocCI, VkBuffer* vkBuffer, VmaAllocation* vmaAllocation,
        VmaAllocationInfo* allocInfo);

    VkResult FlushAllocation(VmaAllocation allocation, u32 offset, u32 size) { return vmaFlushAllocation(vkAllocator_, allocation, offset, size); }

    [[nodiscard]] u32 FindMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const;
    [[nodiscard]] vk::PhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
    [[nodiscard]] vk::Device GetDevice() const { return device_; }
    [[nodiscard]] const QueueFamilies& GetQueues() const { return queueFamilies_; }
    [[nodiscard]] vk::Queue GetPresentQueue() const { return presentQueue_; }
    [[nodiscard]] SemaphoreHandle CreateSemaphore(const vk::SemaphoreCreateInfo& info);

    template <typename... Args> [[nodiscard]] SemaphoreHandleUnique CreateSemaphoreUnique(Args&&... args) {
        return SemaphoreHandleUnique{ CreateSemaphore(std::forward<Args>(args)...), this };
    }
    [[nodiscard]] vk::Semaphore GetSemaphore(SemaphoreHandle handle) const;
    [[nodiscard]] FenceHandle CreateFence(const vk::FenceCreateInfo& info);
    template <typename... Args> [[nodiscard]] FenceHandleUnique CreateFenceUnique(Args&&... args) {
        return FenceHandleUnique{ CreateFence(std::forward<Args>(args)...), this };
    }
    [[nodiscard]] vk::Fence GetFence(FenceHandle handle) const;
    [[nodiscard]] VulkanBuffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    void DestroyBuffer(VulkanBuffer&& buffer);

    [[nodiscard]] VulkanBuffer CopyBufferData(
        vk::CommandBuffer cmd, VulkanBuffer& dstBuffer, const void* srcData, vk::DeviceSize size, vk::DeviceSize offset = 0);
    [[nodiscard]] VulkanBuffer CopyImageData(
        vk::CommandBuffer cmd, VulkanImage& dstTexture, ArrayProxy<SubresourceData> initialData, vk::ImageLayout dstLayout);

    void GenerateMipMaps(
        vk::CommandBuffer cmd, vk::Image image, u32 layers, u32 mipLevels, u32 width, u32 height, vk::ImageLayout initLayout, vk::ImageLayout finalLayout);

    void CreateImageViews(VulkanImage& texture);

    [[nodiscard]] void Transition(vk::CommandBuffer cmd, VulkanImage& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor) const;

    template <typename T> void SetDebugName(T t, std::string_view name) {
        return SetDebugName(t.objectType, reinterpret_cast<u64>(static_cast<typename T::CType>(t)), name);
    }

    void SetDebugName(vk::ObjectType type, u64 handle, std::string_view name);

    [[nodiscard]] ParsedShaderInfo ParseShader(const ShaderParsedData& parser, u32 pushDescriptorDataset);

    void SubmitCommandLists(vk::Semaphore* waitSemaphores = nullptr, const vk::PipelineStageFlags* waitStages = nullptr, u32 waitSemaphoresCount = 0,
        vk::Semaphore* signalSemaphores = nullptr, u32 signalSemaphoresCount = 0, vk::Fence fence = {});

    // Device::*
    void WaitIdle() override;

    void SetDebugName(TextureHandle texture, const char* name) override;

    u64 GetUniformBufferOffsetAlignment() const override { return properties_.limits.minUniformBufferOffsetAlignment; }
    u64 GetUniformBufferSize() const override { return properties_.limits.maxUniformBufferRange; }

    CommandList* BeginCommandList(CommandList::Type type, bool gfxQueue) override;
    void SubmitCommandLists() override;

    Swapchain* GetSwapchain() override;

    RenderPassHandle CreateRenderPass(const RenderPassDesc& desc) override;
    void DestroyRenderPass(RenderPassHandle handle) override;

    FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc) override;
    void DestroyFramebuffer(FramebufferHandle handle) override;

    GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    void DestroyGraphicsPipeline(GraphicsPipelineHandle handle) override;

    ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) override;
    void DestroyComputePipeline(ComputePipelineHandle handle) override;

    BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData, size_t initialDataSize) override;
    void DestroyBuffer(BufferHandle handle) override;

    void* GetBufferMapped(BufferHandle buffer) override;

    TextureHandle CreateTexture(const TextureDesc& desc, TextureLayout initialLayout, ArrayProxy<SubresourceData> initialData) override;
    void DestroyTexture(TextureHandle handle) override;
    TextureHandle CreateTextureHandleFromNativePtr(void* nativeApiHandle, const TextureDesc& desc, bool takeOwnership) override;

    TextureAspectFlags GetTextureAspect(TextureHandle texture) const override;

    TextureDesc GetTextureDesc(TextureHandle texture) override;
    i32 GetTextureBindlessIndex(TextureHandle texture) override;
    i32 GetTextureBindlessIndex(TextureHandle texture, TextureAspectFlags aspect) override;

    SamplerHandle CreateSampler(const SamplerDesc& sampler) override;
    void DestroySampler(SamplerHandle handle) override;
    i32 GetSamplerBindlessIndex(SamplerHandle sampler) override;

    QueryPoolHandle CreateQueryPool(const QueryPoolDesc& desc) override;
    void DestroyQueryPool(QueryPoolHandle handle) override;
    Span<const u64> FetchQueryPoolResults(QueryPoolHandle handle) override;
    double GetMicroseconds(u64 timestamp) const override;

    BufferHandle CreateIndexBuffer(const void* indices, size_t size) override;
    BufferHandle CreateVertexBuffer(const void* data, size_t elementSize, size_t elementCount) override;

    void DestroySemaphore(SemaphoreHandle handle) override;
    void DestroyFence(FenceHandle handle) override;

    BindingHandle CreateBinding(const BindingDesc& binding) override;
    void DestroyBinding(BindingHandle binding) override;

    Format SupportedDepthFormat() const override { return depthFormat_; }
    Format SupportedDepthStencilFormat() const override { return depthStencilFormat_; }

private:
    friend class VulkanSwapchain; // TODO:

    struct PerFrameCommands {
        Vector<VulkanCommandList> command;
    };

    void CheckDebug(std::vector<const char*>& deviceExtensions);
    void InitCommandBuffers();
    void InitDebug();
    void InitAllocator();
    void InitPlatform();
    void InitProfiling();
    void InitExtensions();
    void InitBindings();
    void InitSwapchain(const DeviceCreateInfo& info, vk::UniqueSurfaceKHR surface);

    void InitBindless();
    void DestroyBindless();

    // Deffered destroys.
    void FinalizeFrame(u32 index);

    void DestroyRenderPassImmediately(RenderPassHandle handle);
    void DestroyFramebufferImmediately(FramebufferHandle handle);
    void DestroyGraphicsPipelineImmediately(GraphicsPipelineHandle handle);
    void DestroyComputePipelineImmediately(ComputePipelineHandle handle);
    void DestroyBufferImmediately(BufferHandle handle);
    void DestroyTextureImmediately(TextureHandle handle);
    void DestroySamplerImmediately(SamplerHandle handle);
    void DestroyVertexBufferImmediately(BufferHandle handle);
    void DestroyIndexBufferImmediately(BufferHandle handle);
    void DestroySemaphoreImmediately(SemaphoreHandle handle);
    void DestroyFenceImmediately(FenceHandle handle);
    void DestroyBindingImmediately(BindingHandle handle);
    void DestroyQueryPoolImmediately(QueryPoolHandle handle);

    void DestroyCommands();

    //
    PerFrameCommands& FrameCommands();
    u32 ActiveFrame() const;

    AllocatorRef allocator_;

    UniquePtr<VulkanInstance> instance_;
    UniquePtr<VulkanSwapchain> swapchain_;

    vk::PhysicalDevice physicalDevice_;
    vk::Device device_;

    bool destroyDevice_{};

    vk::PhysicalDeviceProperties properties_{};
    vk::SampleCountFlagBits availableSampleCount_{};

    // VMA
    VmaAllocator vkAllocator_{};

    // Debug extension support.
    bool debug_{};
    bool debugExtension_{};

    bool supportBindless_{};
    Format depthFormat_{};
    Format depthStencilFormat_{};

    // Queues.
    QueueFamilies queueFamilies_{};
    vk::Queue graphicsQueue_;
    vk::Queue transferQueue_;
    vk::Queue computeQueue_;
    vk::Queue presentQueue_;

    bool asyncTransfer_{};

    // Commands.
    vk::UniqueCommandPool gfxCommandPool_;
    vk::UniqueCommandPool transferCommandPool_;

    vk::UniqueCommandBuffer profilerCommandBuffer_;

    std::atomic<u32> cmdIndex_;
    std::atomic<u32> cmdStart_;
    Vector<PerFrameCommands> perFrameCommands_;
    u32 maxCommandList_{};

    // Resource storage.
    UniquePtr<VulkanStorage> storage_;
    Vector<ResourceList> perFrameGraveyard_;
    bool destroying_{};

    // DescriptorSetLayout cache.
    vk::UniqueDescriptorSetLayout emptyDsLayout_;
    const bool useDsLayoutCache_{};
    DescriptorSetLayoutCache dsLayoutCache_;

    UniquePtr<DescriptorSetPool> bindingsDsPool_;

    // Bindless.
    UniquePtr<VulkanBindlessPool> bindlessImages_;
    UniquePtr<VulkanBindlessPool> bindlessSamplers_;
    //UniquePtr<VulkanBindlessPool> bindlessBuffers_;

    // TODO:
    vk::UniqueDescriptorPool imguiDescriptorPool_;

#ifdef UGINE_VK_TRACE_ALLOCATIONS
    std::map<u64, Vector<ugine::StackTrace::StackFrame>> traceAllocations_;
#endif // UGINE_VK_TRACE_ALLOCATIONS
};

} // namespace ugine::gfxapi
