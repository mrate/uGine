#include "DepthPrePass.h"

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <ugine/Profile.h>

using namespace ugine::gfxapi;

namespace ugine {

DepthPrePass::DepthPrePass(GraphicsState& state) {
}

void DepthPrePass::RenderDepth(gfxapi::CommandList& cmd, RenderContext& context) {
    PROFILE_EVENT_NC("DepthPrePass", COLOR_PROFILE_GRAPHICS);

    UGINE_GPU_EVENT(cmd, label, "DepthPrePass");

    const std::array<gfxapi::ClearValue, 1> clearValue = {
        gfxapi::ClearValue::DepthStencil(context.clearDepth, context.clearStencil),
    };

    auto fb{ context.state->GetFramebuffer(gfxapi::DynamicFramebuffer{
        .renderPass = context.state->GetRenderPass(GraphicsState::RenderPass::Depth),
        .extent = context.extent,
        .colorAttachmentCount = 0,
        .depth = context.depthBuffer,
    }) };

    cmd.Barrier(ImageBarrier{
        .texture = context.depthBuffer,
        .srcLayout = TextureLayout::Undefined,
        .srcAccess = AccessFlags::None,
        .srcStage = PipelineStageFlags::BottomOfPipe,
        .dstLayout = TextureLayout::Attachment,
        .dstAccess = AccessFlags::DepthStencilAttachmentWrite,
        .dstStage = PipelineStageFlags::EarlyFragmentTests | PipelineStageFlags::LateFragmentTests,
    });

    cmd.BeginRenderPass(fb, gfxapi::Rect2D::FromExtent(context.extent), u32(clearValue.size()), clearValue.data());

    auto viewport{ gfxapi::Viewport::FromExtent(context.extent) };
    cmd.SetViewport(gfxapi::Viewport{ viewport.x, viewport.height, viewport.width, -(viewport.y + viewport.height) });
    cmd.SetScissor(gfxapi::Rect2D::FromExtent(context.extent));

    RenderGeometry(cmd, context, context.state->SHADER_DEPTH_PASS_MASK, false);

    cmd.EndRenderPass();

    cmd.Barrier(ImageBarrier{
        .texture = context.depthBuffer,
        .srcLayout = TextureLayout::Attachment,
        .srcAccess = AccessFlags::DepthStencilAttachmentWrite,
        .srcStage = PipelineStageFlags::EarlyFragmentTests | PipelineStageFlags::LateFragmentTests,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::ComputeShader | PipelineStageFlags::FragmentShader,
    });
}

} // namespace ugine