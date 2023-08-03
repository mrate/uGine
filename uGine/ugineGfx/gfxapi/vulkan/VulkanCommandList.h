#pragma once

#include "VulkanBumpAllocator.h"
#include "VulkanDescriptorSetPool.h"
#include "VulkanResourceList.h"
#include "VulkanResources.h"

#include <gfxapi/CommandList.h>
#include <gfxapi/Handle.h>

#include <vulkan/vulkan.hpp>

#include <array>

#ifdef MemoryBarrier
#undef MemoryBarrier
#endif

namespace ugine::gfxapi {

class VulkanDevice;
class VulkanCommandList;

struct GPUSemaphore {
    std::vector<vk::UniqueSemaphore> semaphore;

    operator bool() const { return !semaphore.empty(); }

    vk::Semaphore Get(u32 index) const;
};

class VulkanCommandList final : public CommandList {
public:
    VulkanCommandList() = default;
    ~VulkanCommandList() = default;

    VulkanCommandList(const VulkanCommandList&) = delete;
    VulkanCommandList& operator=(const VulkanCommandList&) = delete;

    VulkanCommandList(VulkanCommandList&&) = default;

    void Initialize(VulkanDevice* gfx);
    vk::CommandBuffer VkCommandBuffer();

    void InitSemaphore(GPUSemaphore& semaphore);
    void DestroySemaphore(GPUSemaphore& semaphore);

    // CommandList::*
    Device& GetDevice() override;

    void Begin(Type type, bool gfxQueue) override;
    void End() override;

    Type CommandType() const override { return type_; }

    void* NativePtr() override { return cmd_; }

    BumpAllocator* GetBumpAllocator() override { return &allocator_; }

    GpuAllocation AllocateGPU(size_t size) override;
    void FlushAllocations() override;

    vk::PipelineLayout PipelineLayout() const;

    void BeginDebugLabel(StringView name, const ColorRGBA& color) override;
    void EndDebugLabel() override;

    void SetStencilWriteCompareMask(StencilFaceFlags face, u32 write, u32 compare) override;
    void SetStencilWriteMask(StencilFaceFlags flags, u32 value) override;
    void SetStencilCompareMask(StencilFaceFlags flags, u32 value) override;
    void SetStencilReference(StencilFaceFlags flags, u32 reference) override;

    // Barrier.
    void Barrier(const ImageBarrier& barrier) override;
    void Barrier(const MemoryBarrier& barrier) override;
    void Barrier(const BufferBarrier& barrier) override;

    void FlushBarriers() override;

    // Commands.
    void Draw(u32 vertexCount, u32 instanceCount, u32 vertexStart, u32 firstInstance) override;
    void DrawIndexed(u32 indexCount, u32 instanceCount, u32 indexStart, u32 vertexStart, u32 firstInstance) override;
    void DrawIndirect(BufferHandle bufferHandle, u64 offset, u32 drawCount, u32 stride) override;
    void DrawIndexedIndirect(BufferHandle bufferHandle, u64 offset, u32 drawCount, u32 stride) override;

    void Dispatch(u32 x, u32 y, u32 z) override;
    void DispatchIndirect(BufferHandle buffer, u64 offset) override;

    void UpdateBuffer(BufferHandle buffer, u64 offset, u64 size, const void* data) override;

    void BeginRenderPass(FramebufferHandle framebuffer, const Rect2D& scissor, u32 clearColorCount, const ClearValue* clearColor) override;
    void EndRenderPass() override;

    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Rect2D& scissor) override;

    void BindVertexBuffer(BufferHandle bufferHandle, u64 offset) override;
    void BindVertexBuffer(GpuAllocation bufferHandle, u64 offset) override;
    void BindVertexBuffers(BufferHandle vertexH, BufferHandle instanceH, u64 offsetVertex, u64 offsetInstance) override;
    void BindIndexBuffer(BufferHandle bufferHandle, u64 offset, IndexType indexType) override;
    void BindIndexBuffer(GpuAllocation bufferHandle, u64 offset, IndexType indexType) override;

    void PushConstants(ShaderStage stages, u32 offset, u32 size, const void* data) override;

    void CopyBuffer(BufferHandle src, BufferHandle dst, const BufferCopy& range) override;

    // Bindings.
    void ResetDescriptors() /*override*/;
    //void ResetBinding(u32 set, u32 binding) override;
    //void ResetBinding(u32 set, u32 binding, u32 count) override;

    void BindPipeline(GraphicsPipelineHandle pipeline) override;
    void BindPipeline(ComputePipelineHandle pipeline) override;

    void Bind(u32 set, BindingHandle handle) override;

    void BindUniform(u32 set, u32 binding, BufferHandle bufferHandle) override;
    void BindUniform(u32 set, u32 binding, BufferHandle bufferHandle, u64 offset, u64 size) override;
    void BindUniform(u32 set, u32 binding, GpuAllocation gpuAllocation) override;

    void BindDynamicUniform(u32 set, u32 binding, GpuAllocation gpuAllocation, u32 dynammicOffset) override;

    void BindSampler(u32 set, u32 binding, SamplerHandle samplerRef) override;

    void BindImage(u32 set, u32 binding, TextureHandle imageRef) override;
    void BindImage(u32 set, u32 binding, TextureHandle imageRef, TextureAspectFlags aspect) override;
    void BindImages(u32 set, u32 binding, Span<const TextureHandle> imageRef) override;
    void BindImageSampler(u32 set, u32 binding, TextureHandle imageRef, SamplerHandle samplerRef) override;
    void BindImageSampler(u32 set, u32 binding, TextureHandle imageRef, SamplerHandle samplerRef, TextureAspectFlags aspect) override;
    void BindImagesSampler(u32 set, u32 binding, Span<const TextureHandle> imageRefs, SamplerHandle samplerRef) override;
    void BindImagesSampler(u32 set, u32 binding, Span<const TextureHandle> imageRefs, SamplerHandle samplerRef, TextureAspectFlags aspect) override;
    void BindImageStorage(u32 set, u32 binding, TextureHandle imageRef) override;
    void BindImageStorage(u32 set, u32 binding, TextureHandle imageRef, TextureAspectFlags aspect) override;

    void BindStorage(u32 set, u32 binding, GpuAllocation gpuAllocation) override;
    void BindStorage(u32 set, u32 binding, BufferHandle buffer) override;

    u32 WriteTimestamp(QueryPoolHandle queryPool, PipelineStage stage) override;
    void ResetQueryPool(QueryPoolHandle handle) override;

    void FullPipelineBarrier() override;

private:
    struct Dataset {
        bool dirty{};
        vk::DescriptorSet descriptor{};
        vk::DescriptorSetLayout lastLayout{};

        u32 bindings{};
        u32 bufArrayCnt[MaxBindings];
        u32 imgArrayCnt[MaxBindings];

        vk::DescriptorType TYPES[MaxBindings];
        Vector<vk::DescriptorBufferInfo> BUF[MaxBindings];
        Vector<vk::DescriptorImageInfo> IMG[MaxBindings];
    };

    struct DescriptorSetManager {
        UniquePtr<DescriptorSetPool> pool;

        std::array<Dataset, MaxDatasets> dataset;
        u32 boundDatasets{};

        std::array<u32, MaxDatasets * MaxBindings> dynamicOffset;
        u32 dynamicOffsets{};

        bool dirty{};
        bool dynamicOffsetsDirty{};
    };

    struct CommandPool {
        vk::UniqueCommandPool pool;
        vk::UniqueCommandBuffer buffer;
    };

    bool IsGraphics() const { return type_ == VulkanCommandList::Type::Graphics; }

    bool IsCompute() const { return type_ == VulkanCommandList::Type::Compute; }

    void BindDescriptors();
    void BindPipeline(const VulkanPipeline* pipeline);

    void BindStorage(u32 set, u32 binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size);
    void BindUniform(u32 set, u32 binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size);
    void BindUniformDynamic(u32 set, u32 binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size, u32 dynamicOffset);

    //void UpdateDataset(u32 set, u32 binding, vk::DescriptorType type, const vk::DescriptorBufferInfo* buf, u32 count);
    void UpdateDataset(u32 set, u32 binding, vk::DescriptorType type, const vk::DescriptorBufferInfo& buf);

    void UpdateDataset(u32 set, u32 binding, vk::DescriptorType type, Span<const vk::DescriptorImageInfo> img);
    void UpdateDataset(u32 set, u32 binding, vk::DescriptorType type, const vk::DescriptorImageInfo& img) { UpdateDataset(set, binding, type, { &img, 1 }); }

    VulkanDevice* device_{};

    CommandPool graphics_;
    CommandPool compute_;
    CommandPool copy_;
    CommandPool transition_;

    DescriptorSetManager descriptor_{};

    const VulkanPipeline* activePipeline_{};
    vk::RenderPass activeRenderPass_{};

    Type type_{ Type::Graphics };
    vk::CommandBuffer cmd_{};

    Vector<vk::MemoryBarrier2> memoryBarriers_;
    Vector<vk::BufferMemoryBarrier2> bufferBarriers_;
    Vector<vk::ImageMemoryBarrier2> imageBarriers_;

    VulkanBumpAllocator allocator_{};
};

} // namespace ugine::gfxapi
