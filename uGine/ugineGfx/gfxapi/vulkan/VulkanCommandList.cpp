#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include "VulkanInitializers.h"

#include <gfxapi/FramebufferCache.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/Ugine.h>
#include <ugine/Utils.h>

#include <format>

namespace ugine::gfxapi {
//
//vk::Semaphore GPUSemaphore::Get(u32 index) const {
//    UGINE_ASSERT(!semaphore.empty());
//    return *semaphore[index];
//}
//

void VulkanCommandList::Initialize(VulkanDevice* gfx) {
    device_ = gfx;

    static const std::vector<vk::DescriptorPoolSize> PoolSizes = {
        { vk::DescriptorType::eUniformBuffer, 8 },
        { vk::DescriptorType::eUniformBufferDynamic, 8 },
        { vk::DescriptorType::eCombinedImageSampler, 16 },
        { vk::DescriptorType::eStorageBuffer, 8 },
        { vk::DescriptorType::eStorageImage, 8 },
    };

    static const u32 DescriptorCount{ 1024 };
    static const auto BumpAllocatorSize{ 1024 * 1024 };

    {
        vk::CommandPoolCreateInfo ci{};
        ci.queueFamilyIndex = device_->GetQueues().graphics;
        graphics_.pool = device_->GetDevice().createCommandPoolUnique(ci);
        graphics_.buffer = std::move(device_->GetDevice().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
            .commandPool = *graphics_.pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        })[0]);
        //gfx_->SetObjectDebugName(*cmd.gfxCommandBuffer, std::format("GFX Command {} [{}]", i, frameIndex));
    }

    {
        vk::CommandPoolCreateInfo ci{};
        ci.queueFamilyIndex = device_->GetQueues().compute;
        compute_.pool = device_->GetDevice().createCommandPoolUnique(ci);
        compute_.buffer = std::move(device_->GetDevice().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
            .commandPool = *compute_.pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        })[0]);
        //gfx_->SetObjectDebugName(*cmd.computeCommandBuffer, std::format("Compute Command {} [{}]", i, frameIndex));
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (auto& p : PoolSizes) {
        poolSizes.emplace_back(p.type, p.descriptorCount * DescriptorCount);
    }

    descriptor_.pool = MakeUnique<DescriptorSetPool>(device_->Allocator(), *device_, poolSizes, DescriptorCount);

    allocator_ = VulkanBumpAllocator{ *device_ };
    allocator_.Resize(BumpAllocatorSize);

    memoryBarriers_.Reserve(32);
    imageBarriers_.Reserve(32);
    bufferBarriers_.Reserve(32);
}

Device& VulkanCommandList::GetDevice() {
    return *device_;
}

void VulkanCommandList::Begin(Type type, bool gfxQueue) {
    PROFILE_EVENT_NC("Begin CMD", COLOR_PROFILE_GRAPHICS);

    vk::CommandBuffer vkCmd;
    vk::CommandPool vkPool;

    if (type == VulkanCommandList::Type::Graphics || gfxQueue) {
        vkCmd = *graphics_.buffer;
        vkPool = *graphics_.pool;
    } else {
        vkCmd = *compute_.buffer;
        vkPool = *compute_.pool;
    }

    device_->GetDevice().resetCommandPool(vkPool, {});

    type_ = type;
    cmd_ = vkCmd;
    cmd_.begin(vk::CommandBufferBeginInfo{});

    // Prepare descriptors.
    ResetDescriptors();

    descriptor_.pool->Reset();

    activePipeline_ = nullptr;
    activeRenderPass_ = nullptr;
    allocator_.Reset();

    // Dynamic states.
    if (type == VulkanCommandList::Type::Graphics) {
        SetStencilWriteCompareMask(StencilFaceFlags::FrontAndBack, 0x0, 0x0);
    }
}

void VulkanCommandList::End() {
    //PROFILE_GPU_COLLECT(cmd_);

    cmd_.end();
}

GpuAllocation VulkanCommandList::AllocateGPU(size_t size) {
    const size_t alignment{ std::max(
        device_->Properties().limits.minUniformBufferOffsetAlignment, device_->Properties().limits.minStorageBufferOffsetAlignment) };

    return allocator_.Allocate(size, alignment);
}

void VulkanCommandList::FlushAllocations() {
    allocator_.Flush();
}

void VulkanCommandList::FlushBarriers() {
    if (!memoryBarriers_.Empty() || !imageBarriers_.Empty() || !bufferBarriers_.Empty()) {
        cmd_.pipelineBarrier2(vk::DependencyInfo{
            .memoryBarrierCount = u32(memoryBarriers_.Size()),
            .pMemoryBarriers = memoryBarriers_.Data(),
            .bufferMemoryBarrierCount = u32(bufferBarriers_.Size()),
            .pBufferMemoryBarriers = bufferBarriers_.Data(),
            .imageMemoryBarrierCount = u32(imageBarriers_.Size()),
            .pImageMemoryBarriers = imageBarriers_.Data(),
        });

        memoryBarriers_.Clear();
        imageBarriers_.Clear();
        bufferBarriers_.Clear();
    }
}

void VulkanCommandList::BeginDebugLabel(StringView name, const ColorRGBA& color) {
    if (device_->Debug()) {
        vk::DebugUtilsLabelEXT info{};
        info.color[0] = color.r;
        info.color[1] = color.g;
        info.color[2] = color.b;
        info.color[3] = color.a;
        info.pLabelName = name.Data();

        cmd_.beginDebugUtilsLabelEXT(info);
    }
}

void VulkanCommandList::EndDebugLabel() {
    if (device_->Debug()) {
        cmd_.endDebugUtilsLabelEXT();
    }
}

vk::CommandBuffer VulkanCommandList::VkCommandBuffer() {
    UGINE_ASSERT(cmd_);

    return cmd_;
}

void VulkanCommandList::InitSemaphore(GPUSemaphore& semaphore) {
    semaphore.semaphore.resize(device_->FramesInFlight());
    for (auto& s : semaphore.semaphore) {
        s = device_->GetDevice().createSemaphoreUnique({});
    }
}

void VulkanCommandList::DestroySemaphore(GPUSemaphore& semaphore) {
    semaphore.semaphore.clear();
}

vk::PipelineLayout VulkanCommandList::PipelineLayout() const {
    UGINE_ASSERT(activePipeline_);

    return activePipeline_->layout;
}

void VulkanCommandList::Barrier(const MemoryBarrier& barrier) {
    memoryBarriers_.PushBack(vk::MemoryBarrier2{
        .srcStageMask = ToVulkan2(barrier.srcStage),
        .srcAccessMask = ToVulkan2(barrier.srcAccess),
        .dstStageMask = ToVulkan2(barrier.dstStage),
        .dstAccessMask = ToVulkan2(barrier.dstAccess),
    });
}

void VulkanCommandList::Barrier(const ImageBarrier& barrier) {
    auto image{ device_->GetStorage().GetTexture(barrier.texture) };
    UGINE_ASSERT(image);

#ifdef UGINE_VK_TRACE_SYNC
    UGINE_ASSERT(barrier.srcLayout == TextureLayout::Undefined || barrier.srcLayout == image->traceLayout);
    image->traceLayout = barrier.dstLayout;
#endif

    imageBarriers_.PushBack(vk::ImageMemoryBarrier2{
        .srcStageMask = ToVulkan2(barrier.srcStage),
        .srcAccessMask = ToVulkan2(barrier.srcAccess),
        .dstStageMask = ToVulkan2(barrier.dstStage),
        .dstAccessMask = ToVulkan2(barrier.dstAccess),
        .oldLayout = ToVulkan(barrier.srcLayout),
        .newLayout = ToVulkan(barrier.dstLayout),
        .image = image->vkImage,
        .subresourceRange = vk::ImageSubresourceRange {
            .aspectMask = image->vkAspect,
            .baseMipLevel = 0,
            .levelCount = image->desc.mipLevels,
            .baseArrayLayer = 0,
            .layerCount = image->desc.arrayLayers,
        },
    });
}

void VulkanCommandList::Barrier(const BufferBarrier& barrier) {
    auto buffer{ device_->GetStorage().GetBuffer(barrier.allocation ? barrier.allocation->buffer : barrier.buffer) };
    UGINE_ASSERT(buffer);

    bufferBarriers_.PushBack(vk::BufferMemoryBarrier2{
        .srcStageMask = ToVulkan2(barrier.srcStage),
        .srcAccessMask = ToVulkan2(barrier.srcAccess),
        .dstStageMask = ToVulkan2(barrier.dstStage),
        .dstAccessMask = ToVulkan2(barrier.dstAccess),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = buffer->vkBuffer,
        .offset = (barrier.allocation ? barrier.allocation->offset : 0) + barrier.offset,
        .size = barrier.size,
    });
}

void VulkanCommandList::BindPipeline(const VulkanPipeline* pipeline) {
    if (activePipeline_ == pipeline) {
        return;
    }

    activePipeline_ = pipeline;
    cmd_.bindPipeline(activePipeline_->bindPoint, activePipeline_->pipeline);

    descriptor_.dirty = true;
    descriptor_.boundDatasets = 0;
    for (u32 i{}; i < MaxDatasets; ++i) {
        descriptor_.dataset[i].bindings = 0;
        descriptor_.dataset[i].descriptor = nullptr;
        descriptor_.dataset[i].dirty = true;
    }
}

void VulkanCommandList::BindPipeline(GraphicsPipelineHandle pipelineRef) {
    auto pipeline{ device_->GetStorage().GetGraphicsPipeline(pipelineRef) };

    UGINE_ASSERT(pipeline);
    UGINE_ASSERT(pipeline->bindPoint == vk::PipelineBindPoint::eGraphics);

    BindPipeline(pipeline);
}

void VulkanCommandList::BindPipeline(ComputePipelineHandle pipelineRef) {
    auto pipeline{ device_->GetStorage().GetComputePipeline(pipelineRef) };

    UGINE_ASSERT(pipeline);
    UGINE_ASSERT(pipeline->bindPoint == vk::PipelineBindPoint::eCompute);

    BindPipeline(pipeline);
}

void VulkanCommandList::Bind(u32 set, BindingHandle handle) {
    auto binding{ device_->GetStorage().GetBinding(handle) };
    UGINE_ASSERT(binding);

    UGINE_ASSERT(set < MaxDatasets);

    if (descriptor_.dataset[set].descriptor != binding->descriptorSet) {
        descriptor_.dataset[set].descriptor = binding->descriptorSet;
        descriptor_.dataset[set].dirty = false;
        descriptor_.boundDatasets = std::max(descriptor_.boundDatasets, set + 1);
        descriptor_.dirty = true;
    }
}

void VulkanCommandList::Draw(u32 vertexCount, u32 instanceCount, u32 vertexStart, u32 firstInstance) {
    //UGINE_ASSERT(IsGraphics(cmd));

    BindDescriptors();
    cmd_.draw(vertexCount, instanceCount, vertexStart, firstInstance);
    //++FrameStats::Instance().threadStats[0].drawCallsIssued; // TODO: Thread id;
}

void VulkanCommandList::DrawIndexed(u32 indexCount, u32 instanceCount, u32 indexStart, u32 vertexStart, u32 firstInstance) {
    //UGINE_ASSERT(IsGraphics(cmd));

    BindDescriptors();
    cmd_.drawIndexed(indexCount, instanceCount, indexStart, vertexStart, firstInstance);
    //++FrameStats::Instance().threadStats[0].drawCallsIssued; // TODO: Thread id;
}

void VulkanCommandList::DrawIndirect(BufferHandle bufferHandle, u64 offset, u32 drawCount, u32 stride) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    //UGINE_ASSERT(IsGraphics(cmd));

    BindDescriptors();
    cmd_.drawIndirect(buffer->vkBuffer, offset, drawCount, stride);
    //++FrameStats::Instance().threadStats[0].drawCallsIndirect; // TODO: Thread id;
}

void VulkanCommandList::DrawIndexedIndirect(BufferHandle bufferHandle, u64 offset, u32 drawCount, u32 stride) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    //UGINE_ASSERT(IsGraphics(cmd));

    BindDescriptors();
    cmd_.drawIndexedIndirect(buffer->vkBuffer, offset, drawCount, stride);
    //++FrameStats::Instance().threadStats[0].drawCallsIndirect; // TODO: Thread id;
}

//void VulkanCommandList::Draw(const DrawCall& drawCall) {
//    if (drawCall.indexCount > 0) {
//        DrawIndexed(drawCall.indexCount, drawCall.instanceCount, drawCall.indexStart, drawCall.vertexOffset, drawCall.firstInstance);
//    } else {
//        Draw(drawCall.vertexCount, drawCall.instanceCount, drawCall.vertexOffset, drawCall.firstInstance);
//    }
//}

void VulkanCommandList::Dispatch(u32 x, u32 y, u32 z) {
    //UGINE_ASSERT(IsCompute(cmd));
    UGINE_ASSERT(x > 0 && y > 0 && z > 0);

    FlushBarriers();
    BindDescriptors();
    cmd_.dispatch(x, y, z);
    //++FrameStats::Instance().threadStats[0].compDispatches; // TODO: Thread id;
}

void VulkanCommandList::DispatchIndirect(BufferHandle bufferHandle, u64 offset) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    //UGINE_ASSERT(IsCompute(cmd));

    FlushBarriers();
    BindDescriptors();
    cmd_.dispatchIndirect(buffer->vkBuffer, offset);
    //++FrameStats::Instance().threadStats[0].passes; // TODO: Thread id;
}

void VulkanCommandList::UpdateBuffer(BufferHandle bufferH, u64 offset, u64 size, const void* data) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferH) };
    UGINE_ASSERT(buffer);

    UGINE_ASSERT(!activeRenderPass_);

    cmd_.updateBuffer(buffer->vkBuffer, offset, size, data);
}

void VulkanCommandList::BeginRenderPass(FramebufferHandle framebufferH, const Rect2D& scissor, u32 clearColorCount, const ClearValue* clearColor) {

    auto framebuffer{ device_->GetStorage().GetFramebuffer(framebufferH) };
    UGINE_ASSERT(framebuffer);

#ifdef UGINE_VK_TRACE_SYNC
    //framebuffer->
#endif

    std::array<vk::ClearValue, MaxColorAttachments + 1> clearValues;
    for (u32 i{}; i < clearColorCount; ++i) {
        vk::ClearValue cv{};
        if (clearColor[i].flags & ClearFlags::Color) {
            const std::array<f32, 4> c{ clearColor[i].color.r, clearColor[i].color.g, clearColor[i].color.b, clearColor[i].color.a };
            cv.color = vk::ClearColorValue{ c };
        }

        if (clearColor[i].flags & ClearFlags::Depth) {
            cv.depthStencil.depth = clearColor[i].depthStencil.depth;
        }

        if (clearColor[i].flags & ClearFlags::Stencil) {
            cv.depthStencil.stencil = clearColor[i].depthStencil.stencil;
        }

        clearValues[i] = cv;
    }

    FlushBarriers();

    const vk::RenderPassBeginInfo renderPassInfo{
        .renderPass = framebuffer->vkRenderPass,
        .framebuffer = *framebuffer->vkFramebuffer,
        .renderArea = ToVulkan(scissor),
        .clearValueCount = clearColorCount,
        .pClearValues = clearValues.data(),
    };

    //++FrameStats::Instance().threadStats[0].passes; // TODO: Thread id;

    cmd_.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    activeRenderPass_ = framebuffer->vkRenderPass;
}

void VulkanCommandList::EndRenderPass() {
    cmd_.endRenderPass();
    activeRenderPass_ = nullptr;
}

void VulkanCommandList::SetScissor(const Rect2D& scissor) {
    cmd_.setScissor(0, ToVulkan(scissor));
}

void VulkanCommandList::SetStencilWriteCompareMask(StencilFaceFlags face, u32 write, u32 compare) {
    auto& command(cmd_);

    command.setStencilWriteMask(ToVulkan(face), write);
    command.setStencilCompareMask(ToVulkan(face), compare);
}

void VulkanCommandList::SetStencilWriteMask(StencilFaceFlags flags, u32 value) {
    cmd_.setStencilWriteMask(ToVulkan(flags), value);
}

void VulkanCommandList::SetStencilCompareMask(StencilFaceFlags flags, u32 value) {
    cmd_.setStencilCompareMask(ToVulkan(flags), value);
}

void VulkanCommandList::SetStencilReference(StencilFaceFlags flags, u32 reference) {
    cmd_.setStencilReference(ToVulkan(flags), reference);
}

void VulkanCommandList::BindVertexBuffer(BufferHandle bufferH, u64 offset) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferH) };
    UGINE_ASSERT(buffer);

    cmd_.bindVertexBuffers(0, buffer->vkBuffer, offset);
}

void VulkanCommandList::BindVertexBuffers(BufferHandle vertexH, BufferHandle instanceH, u64 offsetVertex, u64 offsetInstance) {
    auto vertex{ device_->GetStorage().GetBuffer(vertexH) };
    auto instance{ device_->GetStorage().GetBuffer(instanceH) };

    UGINE_ASSERT(vertex);
    UGINE_ASSERT(instance);

    vk::Buffer buffers[2] = { vertex->vkBuffer, instance->vkBuffer };
    vk::DeviceSize offsets[2] = { offsetVertex, offsetInstance };

    cmd_.bindVertexBuffers(0, 2, buffers, offsets);
}

void VulkanCommandList::BindVertexBuffer(GpuAllocation alloc, u64 offset) {
    auto buffer{ device_->GetStorage().GetBuffer(alloc.buffer) };
    UGINE_ASSERT(buffer);

    cmd_.bindVertexBuffers(0, buffer->vkBuffer, alloc.offset + offset);
}

void VulkanCommandList::BindIndexBuffer(BufferHandle bufferHandle, u64 offset, IndexType indexType) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    cmd_.bindIndexBuffer(buffer->vkBuffer, offset, ToVulkan(indexType));
}

void VulkanCommandList::BindIndexBuffer(GpuAllocation alloc, u64 offset, IndexType indexType) {
    auto buffer{ device_->GetStorage().GetBuffer(alloc.buffer) };
    UGINE_ASSERT(buffer);

    cmd_.bindIndexBuffer(buffer->vkBuffer, alloc.offset + offset, ToVulkan(indexType));
}

void VulkanCommandList::PushConstants(ShaderStage stages, u32 offset, u32 size, const void* data) {
    UGINE_ASSERT(activePipeline_);

    cmd_.pushConstants(activePipeline_->layout, ToVulkan(stages), offset, size, data);
}

void VulkanCommandList::CopyBuffer(BufferHandle src, BufferHandle dst, const BufferCopy& range) {
    auto bufferSrc{ device_->GetStorage().GetBuffer(src) };
    UGINE_ASSERT(bufferSrc);
    auto bufferDst{ device_->GetStorage().GetBuffer(dst) };
    UGINE_ASSERT(bufferDst);

    cmd_.copyBuffer(bufferSrc->vkBuffer, bufferDst->vkBuffer, ToVulkan(range));
}

void VulkanCommandList::SetViewport(const Viewport& viewport) {
    cmd_.setViewport(0, ToVulkan(viewport));
}

void VulkanCommandList::BindUniform(u32 set, u32 binding, BufferHandle bufferHandle) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    BindUniform(set, binding, buffer->vkBuffer, 0, buffer->size);
}

void VulkanCommandList::BindUniform(u32 set, u32 binding, BufferHandle bufferHandle, u64 offset, u64 size) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    BindUniform(set, binding, buffer->vkBuffer, offset, size);
}

void VulkanCommandList::BindUniform(u32 set, u32 binding, GpuAllocation gpuAllocation) {
    auto buffer{ device_->GetStorage().GetBuffer(gpuAllocation.buffer) };
    UGINE_ASSERT(buffer);

    BindUniform(set, binding, buffer->vkBuffer, gpuAllocation.offset, gpuAllocation.size);
}

void VulkanCommandList::BindUniform(u32 set, u32 binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size) {
    UpdateDataset(set, binding, vk::DescriptorType::eUniformBuffer, vk::DescriptorBufferInfo{ buffer, offset, size });
}

void VulkanCommandList::BindDynamicUniform(u32 set, u32 binding, GpuAllocation gpuAllocation, u32 dynamicOffset) {
    auto buffer{ device_->GetStorage().GetBuffer(gpuAllocation.buffer) };
    UGINE_ASSERT(buffer);

    BindUniformDynamic(set, binding, buffer->vkBuffer, gpuAllocation.offset, gpuAllocation.size, dynamicOffset);
}

void VulkanCommandList::BindSampler(u32 set, u32 binding, SamplerHandle samplerRef) {
    const auto& sampler{ device_->GetStorage().GetSampler(samplerRef) };
    UGINE_ASSERT(sampler);

    vk::DescriptorImageInfo info{
        .sampler = *sampler->sampler,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eSampler, info);
}

void VulkanCommandList::BindUniformDynamic(u32 set, u32 binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size, u32 dynamicOffset) {
    UpdateDataset(set, binding, vk::DescriptorType::eUniformBufferDynamic, vk::DescriptorBufferInfo{ buffer, offset, size });
    descriptor_.dynamicOffset[descriptor_.dynamicOffsets++] = dynamicOffset;
    descriptor_.dirty = true;
}

void VulkanCommandList::BindImage(u32 set, u32 binding, TextureHandle imageRef) {
    const auto& image{ device_->GetStorage().GetTexture(imageRef) };
    UGINE_ASSERT(image);
    UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);
    UGINE_ASSERT(image->View());

    vk::DescriptorImageInfo info{
        .imageView = image->View(),
        .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eSampledImage, info);
}

void VulkanCommandList::BindImage(u32 set, u32 binding, TextureHandle imageRef, TextureAspectFlags aspect) {
    const auto& image{ device_->GetStorage().GetTexture(imageRef) };
    UGINE_ASSERT(image);
    UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);
    UGINE_ASSERT(image->View(aspect));

    vk::DescriptorImageInfo info{
        .imageView = image->View(aspect),
        .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eSampledImage, info);
}

void VulkanCommandList::BindImages(u32 set, u32 binding, Span<const TextureHandle> imageRefs) {
    UGINE_ASSERT(imageRefs.Size() <= 64);

    std::array<vk::DescriptorImageInfo, 64> descriptors;

    for (size_t i{}; i < std::min(imageRefs.Size(), descriptors.size()); ++i) {
        const auto image{ device_->GetStorage().GetTexture(imageRefs[i]) };
        UGINE_ASSERT(image);
        UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);
        UGINE_ASSERT(image->View());

        descriptors[i] = vk::DescriptorImageInfo{
            .imageView = image->View(),
            .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
        };
    }

    UpdateDataset(set, binding, vk::DescriptorType::eSampledImage, Span{ descriptors.data(), imageRefs.Size() });
}

void VulkanCommandList::BindImageStorage(u32 set, u32 binding, TextureHandle imageRef) {
    const auto& image{ device_->GetStorage().GetTexture(imageRef) };
    UGINE_ASSERT(image);
    UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Storage);
    UGINE_ASSERT(image->View());

    vk::DescriptorImageInfo info{
        .imageView = image->View(),
        .imageLayout = vk::ImageLayout::eGeneral,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eStorageImage, info);
}

void VulkanCommandList::BindImageStorage(u32 set, u32 binding, TextureHandle imageRef, TextureAspectFlags aspect) {
    const auto& image{ device_->GetStorage().GetTexture(imageRef) };
    UGINE_ASSERT(image);
    UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Storage);

    vk::DescriptorImageInfo info{
        .imageView = image->View(aspect),
        .imageLayout = vk::ImageLayout::eGeneral,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eStorageImage, info);
}

void VulkanCommandList::BindImageSampler(u32 set, u32 binding, TextureHandle imageRef, SamplerHandle samplerRef) {
    const auto image{ device_->GetStorage().GetTexture(imageRef) };
    UGINE_ASSERT(image);
    UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);

    const auto sampler{ device_->GetStorage().GetSampler(samplerRef) };
    UGINE_ASSERT(sampler);

    vk::DescriptorImageInfo info{
        .sampler = *sampler->sampler,
        .imageView = image->View(),
        .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eCombinedImageSampler, info);
}

void VulkanCommandList::BindImageSampler(u32 set, u32 binding, TextureHandle imageRef, SamplerHandle samplerRef, TextureAspectFlags aspect) {
    const auto image{ device_->GetStorage().GetTexture(imageRef) };
    UGINE_ASSERT(image);
    UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);

    const auto sampler{ device_->GetStorage().GetSampler(samplerRef) };
    UGINE_ASSERT(sampler);

    vk::DescriptorImageInfo info{
        .sampler = *sampler->sampler,
        .imageView = image->View(aspect),
        .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
    };

    UpdateDataset(set, binding, vk::DescriptorType::eCombinedImageSampler, info);
}

void VulkanCommandList::BindImagesSampler(u32 set, u32 binding, Span<const TextureHandle> imageRefs, SamplerHandle samplerRef) {
    UGINE_ASSERT(imageRefs.Size() <= 64);

    std::array<vk::DescriptorImageInfo, 64> descriptors;

    const auto sampler{ device_->GetStorage().GetSampler(samplerRef) };
    UGINE_ASSERT(sampler);

    for (size_t i{}; i < std::min(imageRefs.Size(), descriptors.size()); ++i) {
        const auto image{ device_->GetStorage().GetTexture(imageRefs[i]) };
        UGINE_ASSERT(image);
        UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);

        descriptors[i] = vk::DescriptorImageInfo{
            .sampler = *sampler->sampler,
            .imageView = image->View(),
            .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
        };
    }

    UpdateDataset(set, binding, vk::DescriptorType::eCombinedImageSampler, Span{ descriptors.data(), imageRefs.Size() });
}

void VulkanCommandList::BindImagesSampler(u32 set, u32 binding, Span<const TextureHandle> imageRefs, SamplerHandle samplerRef, TextureAspectFlags aspect) {
    UGINE_ASSERT(imageRefs.Size() <= 64);

    std::array<vk::DescriptorImageInfo, 64> descriptors;

    const auto sampler{ device_->GetStorage().GetSampler(samplerRef) };
    UGINE_ASSERT(sampler);

    for (size_t i{}; i < std::min(imageRefs.Size(), descriptors.size()); ++i) {
        const auto image{ device_->GetStorage().GetTexture(imageRefs[i]) };
        UGINE_ASSERT(image);
        UGINE_ASSERT(image->desc.usage & TextureUsageFlags::Sampled);

        descriptors[i] = vk::DescriptorImageInfo{
            .sampler = *sampler->sampler,
            .imageView = image->View(aspect),
            .imageLayout = vk::ImageLayout::eReadOnlyOptimal,
        };
    }

    UpdateDataset(set, binding, vk::DescriptorType::eCombinedImageSampler, Span{ descriptors.data(), imageRefs.Size() });
}

void VulkanCommandList::BindStorage(u32 set, u32 binding, GpuAllocation gpuAllocation) {
    auto buffer{ device_->GetStorage().GetBuffer(gpuAllocation.buffer) };
    UGINE_ASSERT(buffer);

    BindStorage(set, binding, buffer->vkBuffer, gpuAllocation.offset, gpuAllocation.size);
}

void VulkanCommandList::BindStorage(u32 set, u32 binding, BufferHandle bufferHandle) {
    auto buffer{ device_->GetStorage().GetBuffer(bufferHandle) };
    UGINE_ASSERT(buffer);

    BindStorage(set, binding, buffer->vkBuffer, 0, buffer->size);
}

void VulkanCommandList::BindStorage(u32 set, u32 binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size) {
    UpdateDataset(set, binding, vk::DescriptorType::eStorageBuffer, vk::DescriptorBufferInfo{ buffer, offset, size });
}

u32 VulkanCommandList::WriteTimestamp(QueryPoolHandle queryPoolHandle, PipelineStage stage) {
    auto queryPool{ device_->GetStorage().GetQueryPool(queryPoolHandle) };
    UGINE_ASSERT(queryPool);

    const auto query{ queryPool->Allocate() };
    cmd_.writeTimestamp2(ToVulkan(stage), queryPool->Pool(), query);
    return query;
}

void VulkanCommandList::ResetQueryPool(QueryPoolHandle queryPoolHandle) {
    auto queryPool{ device_->GetStorage().GetQueryPool(queryPoolHandle) };
    UGINE_ASSERT(queryPool);

    queryPool->Reset(cmd_);
}

void VulkanCommandList::FullPipelineBarrier() {
    const vk::MemoryBarrier2 barrier{
        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    };

    const vk::DependencyInfo dependency{
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrier,
    };

    cmd_.pipelineBarrier2(dependency);
}

void VulkanCommandList::UpdateDataset(u32 set, u32 binding, vk::DescriptorType type, const vk::DescriptorBufferInfo& buf) {
    PROFILE_EVENT_NC("UpdateDataset", COLOR_PROFILE_GRAPHICS);

    UGINE_ASSERT(set < MaxDatasets);
    UGINE_ASSERT(binding < MaxBindings);

    auto& dataset{ descriptor_.dataset[set] };

    // TODO:
    descriptor_.dataset[set].BUF[binding].Reserve(1);

    descriptor_.boundDatasets = std::max(descriptor_.boundDatasets, set + 1);

    //if (dataset.BUF[binding][0] != buf) {
    dataset.descriptor = nullptr; // Reset descriptor, will issue new allocation.
    dataset.bindings = std::max(dataset.bindings, binding + 1);
    dataset.TYPES[binding] = type;
    dataset.BUF[binding][0] = buf;
    dataset.bufArrayCnt[binding] = 1;
    dataset.dirty = true;
    descriptor_.dirty = true;
    //}
}

void VulkanCommandList::UpdateDataset(u32 set, u32 binding, vk::DescriptorType type, Span<const vk::DescriptorImageInfo> img) {
    UGINE_ASSERT(set < MaxDatasets);
    UGINE_ASSERT(binding < MaxBindings);
    UGINE_ASSERT(img.Size() < 32);

    auto& dataset{ descriptor_.dataset[set] };

    // TODO:
    descriptor_.dataset[set].IMG[binding].Reserve(1);

    dataset.bindings = std::max(dataset.bindings, binding + 1);

    //if (dataset.IMG[binding][0] != img) {
    dataset.descriptor = nullptr; // Reset descriptor, will issue new allocation.
    dataset.TYPES[binding] = type;

    dataset.IMG[binding].Resize(img.Size());
    for (u32 i{}; i < img.Size(); ++i) {
        dataset.IMG[binding][i] = img[i];
    }
    dataset.imgArrayCnt[binding] = u32(img.Size());

    dataset.dirty = true;
    descriptor_.boundDatasets = std::max(descriptor_.boundDatasets, set + 1);
    descriptor_.dirty = true;
    //}
}

void VulkanCommandList::ResetDescriptors() {
    descriptor_.dirty = true;
    descriptor_.boundDatasets = 0;
    for (u32 i{}; i < MaxDatasets; ++i) {
        auto& dataset{ descriptor_.dataset[i] };

        dataset.descriptor = nullptr;
        dataset.dirty = false;
        dataset.bindings = 0;
        for (u32 j = 0; j < MaxBindings; ++j) {
            dataset.bufArrayCnt[j] = 0;
            dataset.imgArrayCnt[j] = 0;
        }
    }
}
//
//void VulkanCommandList::ResetBinding(u32 set, u32 binding) {
//    UGINE_ASSERT(set < MaxDatasets);
//    UGINE_ASSERT(binding < MaxBindings);
//
//    descriptor_.dirty = true;
//    descriptor_.dataset[set].descriptor = nullptr;
//    descriptor_.dataset[set].dirty = true;
//    descriptor_.dataset[set].bufArrayCnt[binding] = 0;
//    descriptor_.dataset[set].imgArrayCnt[binding] = 0;
//    if (binding == descriptor_.dataset[set].bindings - 1) {
//        --descriptor_.dataset[set].bindings;
//    }
//}
//
//void VulkanCommandList::ResetBinding(u32 set, u32 binding, u32 count) {
//    UGINE_ASSERT(set < MaxDatasets);
//    UGINE_ASSERT(count > 0);
//    UGINE_ASSERT(binding + count < MaxBindings);
//
//    descriptor_.dirty = true;
//    descriptor_.dataset[set].dirty = true;
//    descriptor_.dataset[set].descriptor = nullptr;
//
//    for (u32 i = binding; i < binding + count; ++i) {
//        descriptor_.dataset[set].bufArrayCnt[i] = 0;
//        descriptor_.dataset[set].imgArrayCnt[i] = 0;
//    }
//
//    if (binding + count == descriptor_.dataset[set].bindings) {
//        descriptor_.dataset[set].bindings -= count;
//    }
//}

void VulkanCommandList::BindDescriptors() {
    PROFILE_EVENT_NC("BindDescriptors", COLOR_PROFILE_GRAPHICS);

    if (!descriptor_.dirty) {
        return;
    }

    descriptor_.dirty = false;
    if (descriptor_.boundDatasets == 0 && activePipeline_->bindlessImagesDataset == BindlessInvalid
        && activePipeline_->bindlessSamplerDataset == BindlessInvalid) {
        return;
    }

    UGINE_ASSERT(activePipeline_);

    const auto& dsLayouts{ activePipeline_->descriptorSetLayouts };

    // Prepare descriptors.
    std::array<vk::DescriptorSet, MaxBindings> descriptors;
    u32 descriptorsCount{};
    u32 skippedDescriptors{};

    for (u32 i{}; i < activePipeline_->descriptorSetLayouts.Size(); ++i) {
        if ((activePipeline_->bindingMask.datasets & (1 << i)) == 0) {
            ++skippedDescriptors;
            continue;
        }

        if (i == activePipeline_->bindlessImagesDataset) {
            descriptors[descriptorsCount++] = device_->BindlessImages();
            continue;
        } else if (i == activePipeline_->bindlessSamplerDataset) {
            descriptors[descriptorsCount++] = device_->BindlessSamplers();
            continue;
        }

#ifdef UGINE_VK_PUSH_DESCRIPTORS
        if (i == activePipeline_->pushDescriptorDataset) {
            continue;
        }
#endif
        UGINE_ASSERT(i < descriptor_.boundDatasets);

        auto& dataset{ descriptor_.dataset[i] };

        if (dataset.dirty && (!dataset.descriptor || dataset.lastLayout != activePipeline_->descriptorSetLayouts[i])) {
            dataset.descriptor = descriptor_.pool->Allocate(dsLayouts[i]);
            dataset.lastLayout = activePipeline_->descriptorSetLayouts[i];
        }

        descriptors[descriptorsCount++] = dataset.descriptor;

        UGINE_ASSERT(dataset.descriptor);
        UGINE_ASSERT(descriptorsCount <= descriptors.size());
    }

    UGINE_ASSERT(descriptorsCount == dsLayouts.Size() - (activePipeline_->pushDescriptorDataset < MaxDatasets ? 1 : 0) - skippedDescriptors);

    // Update writes.
    std::array<vk::WriteDescriptorSet, MaxDatasets * MaxBindings> descriptorWrites;
    u32 descriptorWritesCount{};
    u32 datasetDescriptorBegin{};

    for (u32 i{}; i < activePipeline_->descriptorSetLayouts.Size(); ++i) {
        if (i == activePipeline_->bindlessImagesDataset || i == activePipeline_->bindlessSamplerDataset || i == activePipeline_->bindlessBuffersDataset) {
            continue;
        }

        if ((activePipeline_->bindingMask.datasets & (1 << i)) == 0) {
            continue;
        }

        auto& dataset{ descriptor_.dataset[i] };

        datasetDescriptorBegin = descriptorWritesCount;
        if (dataset.dirty) {
            dataset.dirty = false;

            for (u32 binding{}; binding < dataset.bindings; ++binding) {
                if ((activePipeline_->bindingMask.bindings[i] & (1 << binding)) == 0) {
                    continue;
                }

                switch (dataset.TYPES[binding]) {
                case vk::DescriptorType::eUniformBuffer:
                case vk::DescriptorType::eUniformBufferDynamic:
                case vk::DescriptorType::eStorageBuffer:
                    descriptorWrites[descriptorWritesCount++]
                        = DescriptorBuffer(dataset.descriptor, dataset.TYPES[binding], dataset.BUF[binding].Data(), binding, dataset.bufArrayCnt[binding]);
                    break;
                case vk::DescriptorType::eCombinedImageSampler:
                case vk::DescriptorType::eStorageImage:
                case vk::DescriptorType::eSampledImage:
                case vk::DescriptorType::eSampler:
                    descriptorWrites[descriptorWritesCount++]
                        = DescriptorImage(dataset.descriptor, dataset.TYPES[binding], dataset.IMG[binding].Data(), binding, dataset.imgArrayCnt[binding]);
                    break;
                default: UGINE_ASSERT(false && "Invalid binding type"); break;
                }

                UGINE_ASSERT(descriptorWritesCount < descriptorWrites.size());
            }

#ifdef UGINE_VK_PUSH_DESCRIPTORS
            if (activePipeline_->pushDescriptorDataset == i) {
                cmd_.pushDescriptorSetKHR(activePipeline_->bindPoint, activePipeline_->layout, i, descriptorWritesCount - datasetDescriptorBegin,
                    &descriptorWrites[datasetDescriptorBegin]);

                descriptorWritesCount = datasetDescriptorBegin;
            }
#endif
        }
    }

    if (descriptorWritesCount > 0) {
        device_->GetDevice().updateDescriptorSets(descriptorWritesCount, descriptorWrites.data(), 0, nullptr);
    }

    if (descriptorsCount > 0) {
        cmd_.bindDescriptorSets(activePipeline_->bindPoint, activePipeline_->layout, 0, descriptorsCount, descriptors.data(), descriptor_.dynamicOffsets,
            descriptor_.dynamicOffset.data());
    }

    descriptor_.dynamicOffsets = 0;
}

} // namespace ugine::gfxapi
