#include "LightCullingPass.h"

#include <ugine/engine/shaders/Shader_LightCull.h>

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <gfxapi/Device.h>
#include <gfxapi/Initializers.h>

#include <ugine/Profile.h>
#include <ugineTools/Embed.h>

#include <ugine/engine/world/Camera.h>

#include <shaders/fullscreen_vs_hlsl.h>
#include <shaders/lightCullDebug_fs_hlsl.h>
#include <shaders/lightCull_cs_hlsl.h>

using namespace ugine::gfxapi;

namespace ugine {

LightCullingPass::LightCullingPass(GraphicsState& state) {
    lightCullCSO_ = state.device.CreateComputePipelineUnique(ComputePipelineDesc{
        .name = "LightCullingCSO",
        .computeShader = CompiledShader{
            .name = "lightCull_cs",
            .entryPoint = "main",
            .data = lightCull_cs.data,
            .size = lightCull_cs.size,
        },
        .pushDescriptorDataset = 0,
    });

    lightCullDebugPSO_ = state.device.CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
                .name = "SsaoPSO",
                .vertexShader = CompiledShader {
                    .name = "fullscreen_vs",
                    .entryPoint = "main",
                    .data = fullscreen_vs.data,
                    .size = fullscreen_vs.size,
                },
                .fragmentShader = CompiledShader {
                    .name = "lightCullDebug_fs",
                    .entryPoint = "main",
                    .data = lightCullDebug_fs.data,
                    .size = lightCullDebug_fs.size,
                },
                .renderPass = state.GetRenderPass(GraphicsState::RenderPass::ForwardLDR),
            });
}

void LightCullingPass::CullLights(CommandList& cmd, RenderContext& context) {
    PROFILE_EVENT_NC("Light Culling", COLOR_PROFILE_GRAPHICS);

    const auto lightGridExtent{ context.state->LightGridExtent(context.extent) };
    const auto numFrustums{ lightGridExtent.width * lightGridExtent.height };

    UGINE_GPU_EVENT_C(cmd, label, "LightCullingPass lights", 1.0f, 1.0f, 0.0f);

    context.gpuOpaqueLightGrid
        = context.state->GetRtv(lightGridExtent, Format::R32G32_Uint, TextureUsageFlags::Storage | TextureUsageFlags::Sampled, "OpaqueLightGrid");
    context.gpuTransparentLightGrid
        = context.state->GetRtv(lightGridExtent, Format::R32G32_Uint, TextureUsageFlags::Storage | TextureUsageFlags::Sampled, "TransparentLightGrid");

    UGINE_ASSERT(context.gpuTransparentLightGrid != context.gpuOpaqueLightGrid);

    cmd.Barrier(ImageBarrier{
        .texture = context.gpuOpaqueLightGrid,
        .srcLayout = TextureLayout::Undefined,
        .srcAccess = AccessFlags::ShaderRead,
        .srcStage = PipelineStageFlags::AllCommands,
        .dstLayout = TextureLayout::General,
        .dstAccess = AccessFlags::ShaderWrite,
        .dstStage = PipelineStageFlags::ComputeShader,
    });

    cmd.Barrier(ImageBarrier{
        .texture = context.gpuTransparentLightGrid,
        .srcLayout = TextureLayout::Undefined,
        .srcAccess = AccessFlags::ShaderRead,
        .srcStage = PipelineStageFlags::AllCommands,
        .dstLayout = TextureLayout::General,
        .dstAccess = AccessFlags::ShaderWrite,
        .dstStage = PipelineStageFlags::ComputeShader,
    });

    const auto AVERAGE_LIGHTS_PER_TILE{ 200 };

    const auto lightListSize{ numFrustums * AVERAGE_LIGHTS_PER_TILE * sizeof(u32) };

    {
        auto oLightIndexCounter{ cmd.AllocateGPU(sizeof(u32)) };
        *oLightIndexCounter.As<u32>() = 0;

        auto tLightIndexCounter{ cmd.AllocateGPU(sizeof(u32)) };
        *tLightIndexCounter.As<u32>() = 0;

        context.gpuOpaqueLightIndexList = cmd.AllocateGPU(lightListSize);
        context.gpuTransparentLightIndexList = cmd.AllocateGPU(lightListSize);

        glm::uvec3 disp{ 1 + (context.extent.width - 1) / 16, 1 + (context.extent.height - 1) / 16, 1 };

        auto gpuParams{ cmd.AllocateGPU(sizeof(shaders::LightCullParams)) };
        *gpuParams.As<shaders::LightCullParams>() = shaders::LightCullParams{
            .inverseProjection = context.cameraCB.invProj, // TODO: !
            .view = context.cameraCB.view,
            .numThreadGroups = disp,
            .lightsCount = context.globalCB.lightCount,
        };

        cmd.BindPipeline(*lightCullCSO_);

        cmd.BindImage(0, 0, context.depthBuffer, TextureAspectFlags::Depth);
        cmd.BindImageStorage(0, 1, context.gpuOpaqueLightGrid);
        cmd.BindImageStorage(0, 2, context.gpuTransparentLightGrid);
        cmd.BindStorage(0, 3, oLightIndexCounter);
        cmd.BindStorage(0, 4, tLightIndexCounter);
        cmd.BindStorage(0, 5, context.gpuOpaqueLightIndexList);
        cmd.BindStorage(0, 6, context.gpuTransparentLightIndexList);
        cmd.BindStorage(0, 7, context.gpuLightListSB);
        cmd.BindStorage(0, 8, context.gpuLightCullFrustums);
        cmd.BindUniform(0, 9, gpuParams);

        cmd.Dispatch(disp.x, disp.y, disp.z);
        ++context.computeDispatches;
    }

    cmd.Barrier(BufferBarrier{
        .allocation = &context.gpuOpaqueLightIndexList,
        .offset = 0,
        .size = lightListSize,
        .srcAccess = AccessFlags::ShaderWrite,
        .srcStage = PipelineStageFlags::ComputeShader,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });

    cmd.Barrier(BufferBarrier{
        .allocation = &context.gpuTransparentLightIndexList,
        .offset = 0,
        .size = lightListSize,
        .srcAccess = AccessFlags::ShaderWrite,
        .srcStage = PipelineStageFlags::ComputeShader,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });

    cmd.Barrier(ImageBarrier{
        .texture = context.gpuOpaqueLightGrid,
        .srcLayout = TextureLayout::General,
        .srcAccess = AccessFlags::ShaderWrite,
        .srcStage = PipelineStageFlags::ComputeShader,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });

    cmd.Barrier(ImageBarrier{
        .texture = context.gpuTransparentLightGrid,
        .srcLayout = TextureLayout::General,
        .srcAccess = AccessFlags::ShaderWrite,
        .srcStage = PipelineStageFlags::ComputeShader,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });
}

void LightCullingPass::DebugLights(gfxapi::CommandList& cmd, RenderContext& context, bool transparent) {
    UGINE_GPU_EVENT(cmd, label, "DebugCullLights");

    const std::array<gfxapi::ClearValue, 2> clearValue = {
        gfxapi::ClearValue::Color(context.clearColor),
        gfxapi::ClearValue::DepthStencil(context.clearDepth, context.clearStencil),
    };

    auto fb{ context.state->GetFramebuffer(gfxapi::DynamicFramebuffer{
        .renderPass = context.state->GetRenderPass(GraphicsState::RenderPass::ForwardLDR),
        .extent = context.extent,
        .colorAttachmentCount = 1,
        .colorAttachments = { context.postprocessPingPong[context.activePostProcessTexture] },
        .depth = context.depthBuffer,
    }) };

    cmd.BeginRenderPass(fb, gfxapi::Rect2D::FromExtent(context.extent), u32(clearValue.size()), clearValue.data());

    auto viewport{ gfxapi::Viewport::FromExtent(context.extent) };
    cmd.SetViewport(gfxapi::Viewport{ viewport.x, viewport.y, viewport.width, viewport.height });
    cmd.SetScissor(gfxapi::Rect2D::FromExtent(context.extent));

    struct Params {
        glm::uvec2 resolution;
        u32 lightCount;
    } params = {
        .resolution = glm::uvec2{ context.extent.width, context.extent.height },
        .lightCount = context.globalCB.lightCount,
    };

    cmd.BindPipeline(*lightCullDebugPSO_);

    cmd.BindImage(0, 0, transparent ? context.gpuTransparentLightGrid : context.gpuOpaqueLightGrid);
    cmd.PushConstants(ShaderStage::FragmentShader, 0, params);

    cmd.Draw(3, 1, 0, 0);
    ++context.drawCallsCounter;

    cmd.EndRenderPass();
}

} // namespace ugine