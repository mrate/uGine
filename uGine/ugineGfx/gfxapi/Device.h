#pragma once

#include "CommandList.h"
#include "Types.h"

#include <ugine/ArrayProxy.h>
#include <ugine/Memory.h>

struct ImGui_ImplVulkan_InitInfo;

namespace ugine {
struct ColorRGBA;
}

namespace ugine::gfxapi {

class CommandList;
class Swapchain;

struct DeviceCreateInfo {
    struct Version {
        int maj{};
        int min{};
        int patch{};
    };

    struct {
        const char* appName{};
        Version appVer{};
        const char* engineName{};
        Version engineVer{};

        const char** instanceExtensions{};
        u32 instanceExtensionsCount{};
        const char** deviceExtensions{};
        u32 deviceExtensionsCount{};

        u32 maxCommandBuffers{ 16 };
    } vulkan{};

    struct {
        void* hInstance{};
        void* hWnd{};
    } win32;

    bool validationLayers{};
};

class StaticBindingBuilder {
public:
    virtual ~StaticBindingBuilder() = default;

    virtual StaticBindingBuilder& BindUniform(u32 binding, BufferHandle bufferHandle, u64 offset, u64 size) = 0;
    virtual StaticBindingBuilder& BindImageSampler(u32 binding, TextureHandle imageRef, SamplerHandle samplerRef) = 0;
    virtual StaticBindingBuilder& BindStorage(u32 binding, BufferHandle buffer) = 0;

    virtual BindingHandle Create() = 0;
    virtual BindingHandleUnique CreateUnique() = 0;
};

class Device {
public:
    [[nodiscard]] static UniquePtr<Device> Create(const DeviceCreateInfo& info, IAllocator& allocator = IAllocator::Default());

    Device() = default;
    virtual ~Device() = default;

    // TODO:
    virtual void Fill(ImGui_ImplVulkan_InitInfo& info) = 0;
    virtual void* GetNativeRenderPass(RenderPassHandle handle) = 0;

    virtual void SetDebugName(TextureHandle texture, const char* name) = 0;

    // Commands.
    [[nodiscard]] virtual CommandList* BeginCommandList(CommandList::Type type = CommandList::Type::Graphics, bool gfxQueue = false) = 0;
    virtual void SubmitCommandLists() = 0;

    virtual void WaitIdle() = 0;

    // Creation & destruction.
    [[nodiscard]] virtual Swapchain* GetSwapchain() = 0;

    [[nodiscard]] virtual u64 GetUniformBufferOffsetAlignment() const = 0;
    [[nodiscard]] virtual u64 GetUniformBufferSize() const = 0;

    [[nodiscard]] virtual RenderPassHandle CreateRenderPass(const RenderPassDesc& desc) = 0;
    virtual void DestroyRenderPass(RenderPassHandle handle) = 0;

    [[nodiscard]] virtual FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc) = 0;
    virtual void DestroyFramebuffer(FramebufferHandle handle) = 0;

    [[nodiscard]] virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual void DestroyGraphicsPipeline(GraphicsPipelineHandle handle) = 0;

    [[nodiscard]] virtual ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual void DestroyComputePipeline(ComputePipelineHandle handle) = 0;

    [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr, size_t initialDataSize = 0) = 0;
    virtual void DestroyBuffer(BufferHandle handle) = 0;

    [[nodiscard]] virtual void* GetBufferMapped(BufferHandle buffer) = 0;

    [[nodiscard]] virtual TextureHandle CreateTexture(const TextureDesc& desc, TextureLayout initialLayout, ugine::ArrayProxy<SubresourceData> initialData = {})
        = 0;
    virtual void DestroyTexture(TextureHandle handle) = 0;
    [[nodiscard]] virtual TextureHandle CreateTextureHandleFromNativePtr(void* nativeApiHandle, const TextureDesc& desc, bool takeOwnership) = 0;
    [[nodiscard]] virtual TextureAspectFlags GetTextureAspect(TextureHandle texture) const = 0;

    [[nodiscard]] virtual TextureDesc GetTextureDesc(TextureHandle texture) = 0;
    [[nodiscard]] virtual i32 GetTextureBindlessIndex(TextureHandle texture) = 0;
    [[nodiscard]] virtual i32 GetTextureBindlessIndex(TextureHandle texture, TextureAspectFlags aspect) = 0;

    [[nodiscard]] virtual SamplerHandle CreateSampler(const SamplerDesc& sampler) = 0;
    virtual void DestroySampler(SamplerHandle handle) = 0;
    [[nodiscard]] virtual i32 GetSamplerBindlessIndex(SamplerHandle sampler) = 0;

    [[nodiscard]] virtual QueryPoolHandle CreateQueryPool(const QueryPoolDesc& desc) = 0;
    virtual void DestroyQueryPool(QueryPoolHandle handle) = 0;
    [[nodiscard]] virtual Span<const u64> FetchQueryPoolResults(QueryPoolHandle handle) = 0;
    [[nodiscard]] virtual double GetMicroseconds(u64 timestamp) const = 0;

    [[nodiscard]] virtual BufferHandle CreateVertexBuffer(const void* data, size_t elementSize, size_t elementCount) = 0;
    [[nodiscard]] virtual BufferHandle CreateIndexBuffer(const void* indices, size_t size) = 0;

    [[nodiscard]] BufferHandle CreateIndexBuffer(ugine::ArrayProxy<u16> indices) { return CreateIndexBuffer(indices.data(), 2 * indices.size()); }
    [[nodiscard]] BufferHandle CreateIndexBuffer(ugine::ArrayProxy<u32> indices) { return CreateIndexBuffer(indices.data(), 4 * indices.size()); }

    virtual void DestroySemaphore(SemaphoreHandle handle) = 0;
    virtual void DestroyFence(FenceHandle handle) = 0;

    virtual BindingHandle CreateBinding(const BindingDesc& binding) = 0;
    virtual void DestroyBinding(BindingHandle binding) = 0;

    [[nodiscard]] virtual Format SupportedDepthFormat() const = 0;
    [[nodiscard]] virtual Format SupportedDepthStencilFormat() const = 0;

    // Wrappers.
    template <typename T> [[nodiscard]] BufferHandle CreateVertexBuffer(T&& vertices) {
        auto proxy{ ugine::ArrayProxy{ vertices } };
        return CreateVertexBuffer(proxy.data(), proxy.ElementSize, proxy.size());
    }

    template <typename T> [[nodiscard]] BufferHandleUnique CreateVertexBufferUnique(T&& vertices) {
        auto proxy{ ugine::ArrayProxy{ vertices } };
        return BufferHandleUnique{ CreateVertexBuffer(proxy.data(), proxy.ElementSize, proxy.size()), this };
    }

    template <typename T> [[nodiscard]] BufferHandleUnique CreateVertexBufferUnique(const void* data, size_t elementSize, size_t elementCount) {
        return BufferHandleUnique{ CreateVertexBuffer(data, elementSize, elementCount), this };
    }

    template <typename... Args> [[nodiscard]] BufferHandleUnique CreateIndexBufferUnique(Args&&... args) {
        return BufferHandleUnique{ CreateIndexBuffer(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] GraphicsPipelineHandleUnique CreateGraphicsPipelineUnique(Args&&... args) {
        return GraphicsPipelineHandleUnique{ CreateGraphicsPipeline(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] ComputePipelineHandleUnique CreateComputePipelineUnique(Args&&... args) {
        return ComputePipelineHandleUnique{ CreateComputePipeline(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] BufferHandleUnique CreateBufferUnique(Args&&... args) {
        return BufferHandleUnique{ CreateBuffer(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] TextureHandleUnique CreateTextureUnique(Args&&... args) {
        return TextureHandleUnique{ CreateTexture(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] TextureHandleUnique CreateTextureHandleFromNativePtrUnique(Args&&... args) {
        return TextureHandleUnique{ CreateTextureHandleFromNativePtr(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] SamplerHandleUnique CreateSamplerUnique(Args&&... args) {
        return SamplerHandleUnique{ CreateSampler(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] RenderPassHandleUnique CreateRenderPassUnique(Args&&... args) {
        return RenderPassHandleUnique{ CreateRenderPass(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] FramebufferHandleUnique CreateFramebufferUnique(Args&&... args) {
        return FramebufferHandleUnique{ CreateFramebuffer(std::forward<Args>(args)...), this };
    }

    template <typename... Args> [[nodiscard]] QueryPoolHandleUnique CreateQueryPoolUnique(Args&&... args) {
        return QueryPoolHandleUnique{ CreateQueryPool(std::forward<Args>(args)...), this };
    }
};

} // namespace ugine::gfxapi
