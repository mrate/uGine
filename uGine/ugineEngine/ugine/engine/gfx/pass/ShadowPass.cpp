#include "ShadowPass.h"

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <ugine/Profile.h>

using namespace ugine::gfxapi;

namespace ugine {

ShadowPass::ShadowPass(GraphicsState& state) {
    renderPass_ = state.device.CreateRenderPassUnique(RenderPassDesc{
        .name = "ShadowPass",
        .depthAttachment = RenderPassAttachmentDesc{
            .format = state.device.SupportedDepthStencilFormat(), // TODO: For now due to compatibility with DEPTH pass.
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .finalLayout = TextureLayout::Attachment,
        },    
});
}

void ShadowPass::RenderShadows(gfxapi::CommandList& cmd, RenderContext& context) {
    PROFILE_EVENT_NC("ShadowPass", COLOR_PROFILE_GRAPHICS);

    UGINE_GPU_EVENT(cmd, label, "ShadowPass");

    const std::array<gfxapi::ClearValue, 1> clearValue = {
        gfxapi::ClearValue::DepthStencil(1.0f, 0),
    };

    auto fb{ context.state->GetFramebuffer(gfxapi::DynamicFramebuffer{
        .renderPass = *renderPass_,
        .extent = context.extent,
        .colorAttachmentCount = 0,
        .depth = context.depthBuffer,
    }) };

    cmd.Barrier(ImageBarrier{
        .texture = context.depthBuffer,
        .srcLayout = TextureLayout::Undefined,
        .srcAccess = AccessFlags::None,
        .srcStage = PipelineStageFlags::AllCommands,
        .dstLayout = TextureLayout::Attachment,
        .dstAccess = AccessFlags::DepthStencilAttachmentWrite,
        .dstStage = PipelineStageFlags::EarlyFragmentTests | PipelineStageFlags::LateFragmentTests,
    });

    cmd.BeginRenderPass(fb, gfxapi::Rect2D::FromExtent(context.extent), u32(clearValue.size()), clearValue.data());

    auto viewport{ gfxapi::Viewport::FromExtent(context.extent) };
    // Don't flip Y for shadows.
    cmd.SetViewport(gfxapi::Viewport{ viewport.x, viewport.y, viewport.width, viewport.height });
    cmd.SetScissor(gfxapi::Rect2D::FromExtent(context.extent));

    RenderGeometry(cmd, context, context.state->SHADER_DEPTH_PASS_MASK, false);

    cmd.EndRenderPass();

    cmd.Barrier(ImageBarrier{
        .texture = context.depthBuffer,
        .srcLayout = TextureLayout::Attachment,
        .srcAccess = AccessFlags::DepthStencilAttachmentWrite,
        .srcStage = PipelineStageFlags::LateFragmentTests | PipelineStageFlags::EarlyFragmentTests,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });
}

} // namespace ugine