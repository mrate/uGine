#include "SsaoPass.h"

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <ugine/engine/shaders/Shader_SSAO.h>

#include <ugine/Profile.h>

#include <ugineTools/Embed.h>

#include <ugine/engine/engine/CVars.h>

#include <shaders/blur_cs_hlsl.h>
#include <shaders/fullscreen_vs_hlsl.h>
#include <shaders/ssao_fs_hlsl.h>

#include <random>

using namespace ugine::gfxapi;

namespace ugine {

namespace {
    auto& SSAORadius{ CVars::Register("SSAO radius", "SSAO radius", "ssao", CVar::Type::Float, 0.01f, 0.f, 2.f) };
    auto& SSAOBias{ CVars::Register("SSAO bias", "SSAO bias", "ssao", CVar::Type::Float, 0.01f, 0.f, 2.f) };
    auto& SSAOStrength{ CVars::Register("SSAO strength", "SSAO strength", "ssao", CVar::Type::Float, 0.5f, 0.f, 1.f) };
} // namespace

void GenerateRandomKernel(glm::vec4* array, u32 kernelSize) {
    std::uniform_real_distribution<f32> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;

    for (u32 i = 0; i < kernelSize; ++i) {
        glm::vec3 sample{                        //
            randomFloats(generator) * 2.0 - 1.0, //
            randomFloats(generator) * 2.0 - 1.0, //
            -randomFloats(generator)
        };
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        auto scale = f32(i) / f32(kernelSize);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        array[i] = glm::vec4{ sample, 0.0f };
    }
}

SsaoPass::SsaoPass(GraphicsState& state) {
    renderPass_ = state.device.CreateRenderPassUnique(RenderPassDesc{
        .name = "SsaoPass",
        .colorAttachmentCount = 1,
        .colorAttachments = { RenderPassAttachmentDesc{
            .format = FORMAT,
            .finalLayout = TextureLayout::General,
        } },
    });

    // Blur
    blurCSO_ = state.device.CreateComputePipelineUnique(ComputePipelineDesc{
                .name = "BlurCSO",
                .computeShader = CompiledShader {
                    .name = "blur_cs",
                    .entryPoint = "main",
                    .data = blur_cs.data,
                    .size = blur_cs.size,
                },
                .pushDescriptorDataset = 0,
            });

    // SSAO
    ssaoPSO_ = state.device.CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
                .name = "SsaoPSO",
                .vertexShader = CompiledShader {
                    .name = "fullscreen_vs",
                    .entryPoint = "main",
                    .data = fullscreen_vs.data,
                    .size = fullscreen_vs.size,
                },
                .fragmentShader = CompiledShader {
                    .name = "ssao_fs",
                    .entryPoint = "main",
                    .data = ssao_fs.data,
                    .size = ssao_fs.size,
                },
                .renderPass = *renderPass_,
            });
}

void SsaoPass::Render(CommandList& cmd, RenderContext& context) {
    UGINE_ASSERT(context.state);

    PROFILE_EVENT_NC("SsaoPass", COLOR_PROFILE_GRAPHICS);

    const std::array<ClearValue, 1> clearValue = {
        ClearValue::Color(),
    };

    auto aoTextureInput{ context.state->GetRtv(context.extent, FORMAT, TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled, "AoRenderTarget") };

    if (!context.aoTexture) {
        // TODO: Move to GraphicsScene::PrepareRenderData.
        context.aoTexture = context.state->GetRtv(context.extent, FORMAT, TextureUsageFlags::Storage | TextureUsageFlags::Sampled, "AoOutput");

        context.cameraCB.aoTexture = context.state->device.GetTextureBindlessIndex(context.aoTexture);
        context.gpuCameraCB.As<shaders::Camera>()->aoTexture = context.cameraCB.aoTexture;
    }

    { // AO pass
        UGINE_GPU_EVENT_C(cmd, label, "SsaoPass Render", 0.5f, 1, 0.2f);

        auto fb{ context.state->GetFramebuffer(DynamicFramebuffer{
            .renderPass = *renderPass_,
            .extent = context.extent,
            .colorAttachmentCount = 1,
            .colorAttachments = { aoTextureInput },
        }) };

        cmd.BeginRenderPass(fb, Rect2D::FromExtent(context.extent), u32(clearValue.size()), clearValue.data());

        auto viewport{ Viewport::FromExtent(context.extent) };
        cmd.SetViewport(Viewport{ viewport.x, viewport.y, viewport.width, viewport.height });
        cmd.SetScissor(Rect2D::FromExtent(context.extent));

        auto ssaoCB{ cmd.AllocateGPU(sizeof(shaders::Ssao)) };
        auto pSsaoCB{ ssaoCB.As<shaders::Ssao>() };
        pSsaoCB->radius = SSAORadius.Get<float>();
        pSsaoCB->bias = SSAOBias.Get<float>();
        pSsaoCB->strength = SSAOStrength.Get<float>();
        GenerateRandomKernel(pSsaoCB->samples, 32);

        cmd.BindPipeline(*ssaoPSO_);
        cmd.BindImage(0, 0, context.depthBuffer, TextureAspectFlags::Depth);
        cmd.BindSampler(0, 1, *context.state->samplerClampLinearLinear);
        cmd.BindUniform(0, 2, context.gpuCameraCB);
        cmd.BindUniform(0, 3, ssaoCB);

        context.state->FullscreenTriangle(cmd);
        ++context.drawCallsCounter;

        cmd.EndRenderPass();
    }

    cmd.Barrier(ImageBarrier{
        .texture = aoTextureInput,
        .srcLayout = TextureLayout::General,
        .srcAccess = AccessFlags::ColorAttachmentWrite,
        .srcStage = PipelineStageFlags::ColorAttachmentOutput,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::ComputeShader,
    });

    cmd.Barrier(ImageBarrier{
        .texture = context.aoTexture,
        .srcLayout = TextureLayout::Undefined,
        .srcAccess = AccessFlags::ShaderRead,
        .srcStage = PipelineStageFlags::AllCommands,
        .dstLayout = TextureLayout::General,
        .dstAccess = AccessFlags::ShaderWrite,
        .dstStage = PipelineStageFlags::ComputeShader,
    });

    { // Blur pass.
        UGINE_GPU_EVENT_C(cmd, label, "SsaoPass Blur", 0.5f, 1, 0.2f);

        cmd.BindPipeline(*blurCSO_);

        cmd.BindImage(0, 0, aoTextureInput);
        cmd.BindImageStorage(0, 1, context.aoTexture);

        cmd.Dispatch(u32(std::ceil(context.extent.width / f32(16))), u32(std::ceil(context.extent.height / f32(16))), 1);
        ++context.computeDispatches;
    }

    cmd.Barrier(ImageBarrier{
        .texture = context.aoTexture,
        .srcLayout = TextureLayout::General,
        .srcAccess = AccessFlags::ShaderWrite,
        .srcStage = PipelineStageFlags::ComputeShader,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });

    cmd.FlushBarriers();
}

} // namespace ugine
