#include "GraphicsState.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/Shapes.h>

#include <ugine/engine/gfx/pass/DepthPrePass.h>
#include <ugine/engine/gfx/pass/ForwardPass.h>
#include <ugine/engine/gfx/pass/LightCullingPass.h>
#include <ugine/engine/gfx/pass/ShadowPass.h>
#include <ugine/engine/gfx/pass/SsaoPass.h>
#include <ugine/engine/gfx/pass/TonemappingPass.h>

#include <ugine/engine/shaders/Shader_LightCullFrustums.h>
#include <ugine/engine/shaders/Shader_Types.h>

#include <gfxapi/Initializers.h>

#include <ugine/Profile.h>

#include <ugineTools/Embed.h>

#include <shaders/animation_cs_hlsl.h>
#include <shaders/fullscreen_vs_hlsl.h>
#include <shaders/lightCullGenFrustums_cs_hlsl.h>
#include <shaders/outline_fs_hlsl.h>

using namespace ugine::gfxapi;

namespace ugine {

GraphicsState::GraphicsState(Engine& engine, gfxapi::Device& device, u32 framesInFlight)
    : DEPTH_FORMAT{ device.SupportedDepthFormat() }
    , DEPTH_STENCIL_FORMAT{ device.SupportedDepthStencilFormat() }
    , device{ device }
    , framesInFlight{ framesInFlight }
    , framebufferCache{ device, framesInFlight }
    , rtvCache{ device, framesInFlight }
    , shadowMapCache{ device, framesInFlight } //, renderThread{ device }
{

    const char DEPTH_PASS[] = "PASS_DEPTH";
    const char INSTANCED[] = "MATERIAL_INSTANCE";
    const char OPACITY[] = "MATERIAL_OPACITY";
    const char MASKED[] = "MATERIAL_MASKED";

    SHADER_DEPTH_PASS_MASK = UGINE_BIT(shaderVariants.GetVariantIndex(DEPTH_PASS));
    SHADER_INSTANCED_MASK = UGINE_BIT(shaderVariants.GetVariantIndex(INSTANCED));
    SHADER_OPACITY_MASK = UGINE_BIT(shaderVariants.GetVariantIndex(OPACITY));
    SHADER_MASKED_MASK = UGINE_BIT(shaderVariants.GetVariantIndex(MASKED));

    // TODO:
    shadowMapResolution = Extent2D{ 512, 512 };

    // Samplers.
    {
        samplerClampLinearLinear = device.CreateSamplerUnique(Sampler("samplerClampLinearLinear", TextureAddressMode::Clamp, Filter::Linear, Filter::Linear));
        samplerClampNearestNearest
            = device.CreateSamplerUnique(Sampler("samplerClampNearestNearest", TextureAddressMode::Clamp, Filter::Nearest, Filter::Nearest));
        samplerClampBorderBlackNearest = device.CreateSamplerUnique(SamplerDesc{
            .name = "samplerClampBorderBlackNearest",
            .mipmapFilter{ Filter::Nearest },
            .minFilter{ Filter::Nearest },
            .magFilter{ Filter::Nearest },
            .addressU{ TextureAddressMode::Border },
            .addressV{ TextureAddressMode::Border },
            .addressW{ TextureAddressMode::Border },
            .mipLODBias{},
            .maxAnisotropy{ 1.0f },
            .comparisonFunc{ ComparisonFunc::Always },
            .borderColor = { 0, 0, 0, 0 },
        });
        samplerClampBorderWhiteNearest = device.CreateSamplerUnique(SamplerDesc{
            .name = "samplerClampBorderBlackNearest",
            .mipmapFilter{ Filter::Nearest },
            .minFilter{ Filter::Nearest },
            .magFilter{ Filter::Nearest },
            .addressU{ TextureAddressMode::Border },
            .addressV{ TextureAddressMode::Border },
            .addressW{ TextureAddressMode::Border },
            .mipLODBias{},
            .maxAnisotropy{ 1.0f },
            .comparisonFunc{ ComparisonFunc::Always },
            .borderColor = { 1, 1, 1, 1 },
        });
        samplerWrapLinearNearest = device.CreateSamplerUnique(Sampler("samplerWrapLinearNearest", TextureAddressMode::Wrap, Filter::Linear, Filter::Nearest));
    }

    // Textures.
    {
        const u32 COLOR_PINK{ 0xffff00ff };
        const u32 COLOR_WHITE{ 0xffffffff };
        const u32 COLOR_BLACK{ 0xff000000 };

        auto createTexture = [&](u32 color) {
            return device.CreateTextureUnique(
                TextureDesc{
                    .name = "NullTexture",
                    .extent = Extent2D{ 1, 1 },
                    .format = LDR_FORMAT,
                    .usage = TextureUsageFlags::Sampled,
                },
                TextureLayout::ReadOnly,
                SubresourceData{
                    .data = &color,
                    .size = 4,
                    .pitch = 4,
                    .slicePitch = 4,
                });
        };

        auto createTextureCube = [&](u32 color) {
            const u32 colorArray[6] = { color, color, color, color, color, color };

            return device.CreateTextureUnique(
                TextureDesc{
                    .name = "NullTextureCube",
                    .extent = Extent2D{ 1, 1 },
                    .arrayLayers = 6,
                    .format = LDR_FORMAT,
                    .usage = TextureUsageFlags::Sampled,
                    .misc = TextureMiscFlags::Cube,
                },
                TextureLayout::ReadOnly,
                SubresourceData{
                    .data = colorArray,
                    .size = 4,
                    .pitch = 4,
                    .slicePitch = 4,
                });
        };

        textureNull = createTexture(COLOR_PINK);
        textureWhite = createTexture(COLOR_WHITE);
        textureBlack = createTexture(COLOR_BLACK);
        textureCubeBlack = createTextureCube(COLOR_BLACK);

        UGINE_ASSERT(device.GetTextureBindlessIndex(*textureNull) == 0);
    }

    // PSO
    {
        // Skeletal animation
        animationCSO = device.CreateComputePipelineUnique(ComputePipelineDesc{
                .name = "AnimationCSO",
                .computeShader = CompiledShader {
                    .name = "animation_cs",
                    .entryPoint = "main",
                    .data = animation_cs.data,
                    .size = animation_cs.size,
                },
                .pushDescriptorDataset = 0,
            });
    }

    // Light culling.

    {
        genFrustumsCSO = device.CreateComputePipelineUnique(ComputePipelineDesc{ 
            .name = "LightCullingGenFrustumsCSO", 
            .computeShader = CompiledShader{ 
                .name = "lightCullGenFrustum_cs",
                .entryPoint = "main",
                .data = lightCullGenFrustums_cs.data,
                .size = lightCullGenFrustums_cs.size,
            },
            .pushDescriptorDataset = 0,
        });
    }

    // Renderpasses.
    shadowPass = MakeUnique<ShadowPass>(engine.GetAllocator(), *this);
    depthPrePass = MakeUnique<DepthPrePass>(engine.GetAllocator(), *this);
    forwardPass = MakeUnique<ForwardPass>(engine.GetAllocator());
    lightCullingPass = MakeUnique<LightCullingPass>(engine.GetAllocator(), *this);
    aoPass = MakeUnique<SsaoPass>(engine.GetAllocator(), *this);
    tonemappingPass = MakeUnique<TonemappingPass>(engine.GetAllocator(), *this);

    // Geometry.
    {
        auto [vertices, indices]{ CubeVertices(100) };

        // Invert winding.
        for (int i{}; i < indices.size(); i += 3) {
            std::swap(indices[i], indices[i + 2]);
        }

        skyBoxVertexBuffer = device.CreateVertexBufferUnique(vertices);
        skyBoxIndexBuffer = device.CreateIndexBufferUnique(indices);
        skyBoxIndexCount = u32(indices.size());
    }
}

GraphicsState::~GraphicsState() {
    //renderThread.NextFrame(true);
}

gfxapi::RenderPassHandle GraphicsState::GetRenderPass(RenderPass type) {
    auto it{ renderpasses.find(type) };
    if (it == renderpasses.end()) {
        switch (type) {
        case RenderPass::Depth: it = renderpasses.insert(std::make_pair(type, CreateDepthRenderPass())).first; break;
        case RenderPass::ForwardHDR: it = renderpasses.insert(std::make_pair(type, CreateForwardRenderPass(HDR_FORMAT))).first; break;
        case RenderPass::PostprocessHDR: it = renderpasses.insert(std::make_pair(type, CreatePostProcessRenderPass(HDR_FORMAT))).first; break;
        case RenderPass::ForwardLDR: it = renderpasses.insert(std::make_pair(type, CreateForwardRenderPass(LDR_FORMAT))).first; break;
        case RenderPass::PostprocessLDR: it = renderpasses.insert(std::make_pair(type, CreatePostProcessRenderPass(LDR_FORMAT))).first; break;
        default: UGINE_ASSERT(false && "Invalid render pass"); break;
        }
    }
    return *it->second;
}

gfxapi::Extent2D GraphicsState::LightGridExtent(const gfxapi::Extent2D& extent) const {
    return Extent2D{
        .width{ 1 + (extent.width - 1) / 16 },
        .height{ 1 + (extent.height - 1) / 16 },
    };
}

gfxapi::GraphicsPipelineHandleUnique GraphicsState::CreateOutlinePSO(RenderPassHandle renderPass) {
    return device.CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
                .name = "OutlinePSO",
                .vertexShader = CompiledShader {
                    .name = "fullscreen_vs",
                    .entryPoint = "main",
                    .data = fullscreen_vs.data,
                    .size = fullscreen_vs.size,
                },
                .fragmentShader = CompiledShader {
                    .name = "fullscreen_fs",
                    .entryPoint = "main",
                    .data = outline_fs.data,
                    .size = outline_fs.size,
                },
                .rasterizerState = {
                    .frontCCW = false,
                    .cullMode = CullMode::Back,
                    .fillMode = FillMode::Fill,
                    },
                .renderPass = renderPass,
            });
}

gfxapi::RenderPassHandleUnique GraphicsState::CreateDepthRenderPass() {
    return device.CreateRenderPassUnique(RenderPassDesc{ 
        .name = "DepthRP",
        .depthAttachment = RenderPassAttachmentDesc{
            .format = device.SupportedDepthStencilFormat(),
            .loadOp = AttachmentLoadOp::Clear,
            .storeOp = AttachmentStoreOp::Store,
            .finalLayout = TextureLayout::Attachment,
        }, });
}

gfxapi::RenderPassHandleUnique GraphicsState::CreateForwardRenderPass(gfxapi::Format format) {
    return device.CreateRenderPassUnique(RenderPassDesc{
        .name = "ForwardRP",
        .colorAttachmentCount = 1,
        .colorAttachments = { RenderPassAttachmentDesc{
            .format = format,
            .finalLayout = TextureLayout::ReadOnly,
        }},
        .depthAttachment = RenderPassAttachmentDesc {            
            .format = device.SupportedDepthStencilFormat(),
            .loadOp = AttachmentLoadOp::Load,
            .storeOp = AttachmentStoreOp::Store,
            .stencilLoadOp = AttachmentLoadOp::Clear,
            .stencilStoreOp = AttachmentStoreOp::Store,
            .initialLayout = TextureLayout::Attachment,
            .finalLayout = TextureLayout::ReadOnly,
        },
    });
}

gfxapi::RenderPassHandleUnique GraphicsState::CreatePostProcessRenderPass(gfxapi::Format format) {
    return device.CreateRenderPassUnique(RenderPassDesc{
        .name = "PostProcessRP",
        .colorAttachmentCount = 1,
        .colorAttachments = { RenderPassAttachmentDesc{
            .format = format,
            .finalLayout = TextureLayout::ReadOnly,
        }, },
    });
}

void GraphicsState::CalcLightFrustums(
    gfxapi::CommandList& cmd, gfxapi::BufferHandle frustums, const gfxapi::Extent2D& extent, const glm::mat4& invProjection) const {
    UGINE_GPU_EVENT_C(cmd, label, "LightCullingPass frustums", 1, 0, 1);

    const Extent2D lightGridExtent{
        .width{ 1 + (extent.width - 1) / LIGHT_CULL_BLOCK_SIZE },
        .height{ 1 + (extent.height - 1) / LIGHT_CULL_BLOCK_SIZE },
    };

    shaders::LightCullFrustumParams params{
        .inverseProjection = invProjection, // TODO: !
        .screenResolution = glm::uvec2{ extent.width, extent.height },
        .dispatchResolution = glm::uvec2{ lightGridExtent.width, lightGridExtent.height },
    };

    cmd.BindPipeline(*genFrustumsCSO);

    cmd.BindStorage(0, 0, frustums);
    cmd.PushConstants(ShaderStage::ComputeShader, 0, params);

    cmd.Dispatch(1 + (lightGridExtent.width - 1) / 16, 1 + (lightGridExtent.height - 1) / 16, 1);

    cmd.Barrier(BufferBarrier{
        .buffer = frustums,
        .offset = 0,
        .size = sizeof(shaders::Frustum) * lightGridExtent.width * lightGridExtent.height,
        .srcAccess = AccessFlags::ShaderWrite,
        .srcStage = PipelineStageFlags::ComputeShader,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::ComputeShader,
    });
}

void GraphicsState::FullscreenTriangle(gfxapi::CommandList& cmd) {
    // Fullscreen triangle.
    cmd.Draw(3, 1, 0, 0);
}

} // namespace ugine
