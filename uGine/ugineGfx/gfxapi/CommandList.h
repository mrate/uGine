#pragma once

#include "BumpAllocator.h"
#include "Handle.h"
#include "Types.h"

#include <ugine/Color.h>
#include <ugine/Span.h>
#include <ugine/String.h>

namespace ugine::gfxapi {

struct ImageBarrier {
    TextureHandle texture{};
    TextureLayout srcLayout{};
    AccessFlags srcAccess{};
    PipelineStageFlags srcStage{};
    TextureLayout dstLayout{};
    AccessFlags dstAccess{};
    PipelineStageFlags dstStage{};
};

struct BufferBarrier {
    BufferHandle buffer{};
    const GpuAllocation* allocation{};
    u64 offset{};
    u64 size{};
    AccessFlags srcAccess{};
    PipelineStageFlags srcStage{};
    AccessFlags dstAccess{};
    PipelineStageFlags dstStage{};
};

struct MemoryBarrier {
    AccessFlags srcAccess{};
    PipelineStageFlags srcStage{};
    AccessFlags dstAccess{};
    PipelineStageFlags dstStage{};
};

enum class ClearFlags : u32 {
    Color = 1 << 0,
    Depth = 1 << 1,
    Stencil = 1 << 2,
};

UGINE_FLAGS(ClearFlags, u32);

struct ClearValue {
    static ClearValue Color(f32 r = 0, f32 g = 0, f32 b = 0, f32 a = 1) {
        ClearValue res{};
        res.color.r = r;
        res.color.g = g;
        res.color.b = b;
        res.color.a = a;
        res.flags = ClearFlags::Color;
        return res;
    }

    static ClearValue Color(const ColorRGBA& color) {
        ClearValue res{};
        res.color = color;
        res.flags = ClearFlags::Color;
        return res;
    }

    static ClearValue DepthStencil(f32 depth = 1.0f, u8 stencil = 0) {
        ClearValue res{};
        res.depthStencil.depth = depth;
        res.depthStencil.stencil = stencil;
        res.flags = ClearFlags::Depth | ClearFlags::Stencil;
        return res;
    }

    static ClearValue ColorDepthStencil(f32 r, f32 g, f32 b, f32 a, f32 depth, u8 stencil) {
        ClearValue res{};
        res.color.r = r;
        res.color.g = g;
        res.color.b = b;
        res.color.a = a;
        res.depthStencil.depth = depth;
        res.depthStencil.stencil = stencil;
        res.flags = ClearFlags::Color | ClearFlags::Depth | ClearFlags::Stencil;
        return res;
    }

    ClearFlags flags{};
    ColorRGBA color{};
    struct {
        f32 depth{};
        u8 stencil{};
    } depthStencil;
};

class CommandList {
public:
    enum class Type {
        Graphics = 0,
        Compute,
    };

    virtual ~CommandList() = default;

    virtual Device& GetDevice() = 0;

    virtual void Begin(Type type, bool gfxQueue) = 0;
    virtual void End() = 0;

    virtual Type CommandType() const = 0;

    virtual void* NativePtr() = 0;
    virtual BumpAllocator* GetBumpAllocator() = 0;

    virtual GpuAllocation AllocateGPU(size_t size) = 0;
    virtual void FlushAllocations() = 0;

    virtual void BeginDebugLabel(StringView name, const ColorRGBA& color) = 0;
    virtual void EndDebugLabel() = 0;

    virtual void SetStencilWriteCompareMask(StencilFaceFlags face, u32 write, u32 compare) = 0;
    virtual void SetStencilWriteMask(StencilFaceFlags flags, u32 value) = 0;
    virtual void SetStencilCompareMask(StencilFaceFlags flags, u32 value) = 0;
    virtual void SetStencilReference(StencilFaceFlags flags, u32 reference) = 0;

    // Barriers.
    virtual void Barrier(const MemoryBarrier& barrier) = 0;
    virtual void Barrier(const ImageBarrier& barrier) = 0;
    virtual void Barrier(const BufferBarrier& barrier) = 0;

    // DEBUG:
    virtual void FullPipelineBarrier() = 0;

    virtual void FlushBarriers() = 0;

    // Commands.
    virtual void Draw(u32 vertexCount, u32 instanceCount, u32 vertexStart, u32 firstInstance) = 0;
    virtual void DrawIndexed(u32 indexCount, u32 instanceCount, u32 indexStart, u32 vertexStart, u32 firstInstance) = 0;
    virtual void DrawIndirect(BufferHandle bufferHandle, u64 offset, u32 drawCount, u32 stride) = 0;
    virtual void DrawIndexedIndirect(BufferHandle bufferHandle, u64 offset, u32 drawCount, u32 stride) = 0;

    virtual void Dispatch(u32 x, u32 y, u32 z) = 0;
    virtual void DispatchIndirect(BufferHandle buffer, u64 offset) = 0;

    virtual void UpdateBuffer(BufferHandle buffer, u64 offset, u64 size, const void* data) = 0;

    virtual void BeginRenderPass(FramebufferHandle framebuffer, const Rect2D& scissor, u32 clearColorCount, const ClearValue* clearColor) = 0;
    virtual void EndRenderPass() = 0;

    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissor(const Rect2D& scissor) = 0;

    virtual void PushConstants(ShaderStage stages, u32 offset, u32 size, const void* data) = 0;

    template <typename T> void PushConstants(ShaderStage stages, u32 offset, const T& value) { PushConstants(stages, offset, sizeof(T), &value); }

    virtual void BindVertexBuffer(BufferHandle bufferHandle, u64 offset) = 0;
    virtual void BindVertexBuffer(GpuAllocation bufferHandle, u64 offset) = 0;
    virtual void BindVertexBuffers(BufferHandle vertexH, BufferHandle instanceH, u64 offsetVertex, u64 offsetInstance) = 0;

    virtual void BindIndexBuffer(BufferHandle bufferHandle, u64 offset, IndexType indexType) = 0;
    virtual void BindIndexBuffer(GpuAllocation bufferHandle, u64 offset, IndexType indexType) = 0;

    virtual void CopyBuffer(BufferHandle src, BufferHandle dst, const BufferCopy& range) = 0;

    virtual void BindPipeline(GraphicsPipelineHandle pipeline) = 0;
    virtual void BindPipeline(ComputePipelineHandle pipeline) = 0;

    virtual void Bind(u32 set, BindingHandle handle) = 0;

    virtual void BindUniform(u32 set, u32 binding, BufferHandle buffer) = 0;
    virtual void BindUniform(u32 set, u32 binding, BufferHandle bufferHandle, u64 offset, u64 size) = 0;
    virtual void BindUniform(u32 set, u32 binding, GpuAllocation gpuAllocation) = 0;

    virtual void BindDynamicUniform(u32 set, u32 binding, GpuAllocation gpuAllocation, u32 offset) = 0;

    virtual void BindSampler(u32 set, u32 binding, SamplerHandle samplerRef) = 0;

    virtual void BindImage(u32 set, u32 binding, TextureHandle imageRef) = 0;
    virtual void BindImage(u32 set, u32 binding, TextureHandle imageRef, TextureAspectFlags aspect) = 0;
    virtual void BindImages(u32 set, u32 binding, Span<const TextureHandle> imageRef) = 0;
    virtual void BindImageSampler(u32 set, u32 binding, TextureHandle imageRef, SamplerHandle samplerRef) = 0;
    virtual void BindImageSampler(u32 set, u32 binding, TextureHandle imageRef, SamplerHandle samplerRef, TextureAspectFlags aspect) = 0;
    virtual void BindImagesSampler(u32 set, u32 binding, Span<const TextureHandle> imageRefs, SamplerHandle samplerRef) = 0;
    virtual void BindImagesSampler(u32 set, u32 binding, Span<const TextureHandle> imageRefs, SamplerHandle samplerRef, TextureAspectFlags aspect) = 0;

    virtual void BindImageStorage(u32 set, u32 binding, TextureHandle imageRef) = 0;
    virtual void BindImageStorage(u32 set, u32 binding, TextureHandle imageRef, TextureAspectFlags aspect) = 0;

    virtual void BindStorage(u32 set, u32 binding, GpuAllocation gpuAllocation) = 0;
    virtual void BindStorage(u32 set, u32 binding, BufferHandle buffer) = 0;

    virtual u32 WriteTimestamp(QueryPoolHandle queryPool, PipelineStage stage) = 0;
    virtual void ResetQueryPool(QueryPoolHandle handle) = 0;
};

struct GPUDebugLabel {
    GPUDebugLabel(CommandList& cmd, StringView name, const ColorRGBA& color = ColorRGBA{ 1, 0, 0, 1 })
        : cmd_{ cmd } {
        cmd.BeginDebugLabel(name, color);
    }

    GPUDebugLabel(CommandList& cmd, StringView name, f32 r, f32 g, f32 b, f32 a = 1)
        : cmd_{ cmd } {
        cmd.BeginDebugLabel(name, ColorRGBA{ r, g, b, a });
    }

    ~GPUDebugLabel() { cmd_.EndDebugLabel(); }

private:
    CommandList& cmd_;
};

} // namespace ugine::gfxapi