#include "ForwardPass.h"

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <ugine/Profile.h>

using namespace ugine::gfxapi;

namespace ugine {

void ForwardPass::Render(CommandList& cmd, RenderContext& context) {
    PROFILE_EVENT_NC("ForwardPass", COLOR_PROFILE_GRAPHICS);

    UGINE_GPU_EVENT_C(cmd, label, "ForwardPass", 0.75f, 0.25f, 0.5f);

    const std::array<ClearValue, 2> clearValue = {
        ClearValue::Color(context.clearColor),
        ClearValue::DepthStencil(context.clearDepth, context.clearStencil),
    };

    auto fb{ context.state->GetFramebuffer(DynamicFramebuffer{
        .renderPass = context.state->GetRenderPass(GraphicsState::RenderPass::ForwardHDR),
        .extent = context.extent,
        .colorAttachmentCount = 1,
        .colorAttachments = { context.renderTargetHDR },
        .depth = context.depthBuffer,
    }) };

    cmd.BeginRenderPass(fb, Rect2D::FromExtent(context.extent), u32(clearValue.size()), clearValue.data());

    auto viewport{ Viewport::FromExtent(context.extent) };
    cmd.SetViewport(Viewport{ viewport.x, viewport.height, viewport.width, -(viewport.y + viewport.height) });
    cmd.SetScissor(Rect2D::FromExtent(context.extent));

    RenderGeometry(cmd, context, 0, false);
    RenderGeometry(cmd, context, 0, true);

    cmd.EndRenderPass();
}

} // namespace ugine