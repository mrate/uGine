#include "TonemappingPass.h"

#include <ugine/Profile.h>

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <ugine/engine/shaders/Shader_Tonemapping.h>

#include <ugineTools/Embed.h>

#include <shaders/fullscreen_vs_hlsl.h>
#include <shaders/tonemapping_fs_hlsl.h>

using namespace ugine::gfxapi;

namespace ugine {

TonemappingPass::TonemappingPass(GraphicsState& state) {
    renderPass_ = state.device.CreateRenderPassUnique(RenderPassDesc{
        .name = "TonemappingPass",
        .colorAttachmentCount = 1,
        .colorAttachments = { RenderPassAttachmentDesc{
            .format = state.LDR_FORMAT,
            .finalLayout = TextureLayout::ReadOnly,
        } },
    });

    // SSAO
    tonemappingPSO_ = state.device.CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
                .name = "TonemappingPSO",
                .vertexShader = CompiledShader {
                    .name = "fullscreen_vs",
                    .entryPoint = "main",
                    .data = fullscreen_vs.data,
                    .size = fullscreen_vs.size,
                },
                .fragmentShader = CompiledShader {
                    .name = "tonemapping_fs",
                    .entryPoint = "main",
                    .data = tonemapping_fs.data,
                    .size = tonemapping_fs.size,
                },
                .renderPass = *renderPass_,
            });
}

void TonemappingPass::Render(gfxapi::CommandList& cmd, RenderContext& context) {
    UGINE_ASSERT(context.state);

    PROFILE_EVENT_NC("TonemappingPass", COLOR_PROFILE_GRAPHICS);

    const std::array<ClearValue, 1> clearValue = {
        ClearValue::Color(),
    };

    { // Tonemapping pass
        UGINE_GPU_EVENT_C(cmd, label, "TonemappingPass Render", 0.75f, 1, 0.1f);

        context.activePostProcessTexture = 1 - context.activePostProcessTexture;

        auto input{ context.renderTargetHDR };
        auto output{ context.postprocessPingPong[context.activePostProcessTexture] };

        if (!output) {
            output = context.state->GetRtv(
                context.extent, context.state->LDR_FORMAT, TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled, "TonemappingRenderTarget");
            context.postprocessPingPong[context.activePostProcessTexture] = output;
        }

        auto fb{ context.state->GetFramebuffer(DynamicFramebuffer{
            .renderPass = *renderPass_,
            .extent = context.extent,
            .colorAttachmentCount = 1,
            .colorAttachments = { output },
        }) };

        cmd.BeginRenderPass(fb, Rect2D::FromExtent(context.extent), u32(clearValue.size()), clearValue.data());

        auto viewport{ Viewport::FromExtent(context.extent) };
        cmd.SetViewport(Viewport{ viewport.x, viewport.y, viewport.width, viewport.height });
        cmd.SetScissor(Rect2D::FromExtent(context.extent));

        auto uboCB{ cmd.AllocateGPU(sizeof(shaders::Tonemapping)) };
        auto pUbo{ uboCB.As<shaders::Tonemapping>() };

        // TODO: Exposure params.
        pUbo->algorithm = 1;
        pUbo->exposure = 1;
        pUbo->gamma = 1;

        cmd.BindPipeline(*tonemappingPSO_);
        cmd.BindImage(0, 0, input);
        cmd.BindSampler(0, 1, *context.state->samplerClampNearestNearest);
        cmd.BindUniform(0, 2, uboCB);

        context.state->FullscreenTriangle(cmd);
        ++context.drawCallsCounter;

        cmd.EndRenderPass();

        //cmd.Barrier(ImageBarrier{
        //    .texture = output,
        //    .srcLayout = TextureLayout::Attachment,
        //    .srcAccess = AccessFlags::ColorAttachmentWrite,
        //    .srcStage = PipelineStageFlags::ColorAttachmentOutput,
        //    .dstLayout = TextureLayout::ReadOnly,
        //    .dstAccess = AccessFlags::ShaderRead,
        //    .dstStage = PipelineStageFlags::FragmentShader,
        //});
    }
}

} // namespace ugine