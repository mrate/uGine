#pragma once

#include <gfxapi/Device.h>
#include <gfxapi/FramebufferCache.h>
#include <gfxapi/RenderTargetCache.h>

#include <ugine/Memory.h>

#include "Material.h"
#include "RenderThread.h"
#include "Shader.h"

#include <map>
#include <memory>
#include <vector>

namespace ugine {

class DepthPrePass;
class ForwardPass;
class LightCullingPass;
class ShadowPass;
class SsaoPass;
class TonemappingPass;

// TODO: Basically a Renderer...
class GraphicsState {
public:
    enum class RenderPass {
        Depth,
        ForwardHDR,
        ForwardLDR,
        PostprocessHDR,
        PostprocessLDR,
    };

    inline static const auto HDR_FORMAT{ gfxapi::Format::R32G32B32A32_Float };
    inline static const auto LDR_FORMAT{ gfxapi::Format::R8G8B8A8_Unorm };
    const gfxapi::Format DEPTH_FORMAT;
    const gfxapi::Format DEPTH_STENCIL_FORMAT;

    //inline static const auto SHADOWMAP_FORMAT{ gfxapi::Format::D32_Float };

    GraphicsState(Engine& engine, gfxapi::Device& device, u32 framesInFlight);
    ~GraphicsState();

    gfxapi::Device& device;
    const u32 framesInFlight;

    u32 ActiveFrameInFlight() const { return frameNumber % framesInFlight; }
    u64 frameNumber{};

    void FullscreenTriangle(gfxapi::CommandList& cmd);

    // TODO:
    gfxapi::TextureHandle mainOutputOverride;

    // TODO:
    // Render-pass
    gfxapi::RenderPassHandle GetRenderPass(RenderPass type);

    // Light culling.
    void CalcLightFrustums(gfxapi::CommandList& cmd, gfxapi::BufferHandle frustums, const gfxapi::Extent2D& extent, const glm::mat4& invProjection) const;
    gfxapi::Extent2D LightGridExtent(const gfxapi::Extent2D& extent) const;

    // TODO:
    gfxapi::GraphicsPipelineHandleUnique CreateOutlinePSO(gfxapi::RenderPassHandle renderPass);

    // Cache
    gfxapi::FramebufferHandle GetFramebuffer(const gfxapi::DynamicFramebuffer& key) { return framebufferCache.Get(frameNumber, key); }
    gfxapi::TextureHandle GetRtv(const gfxapi::Extent2D& extent, gfxapi::Format format,
        gfxapi::TextureUsageFlags usage = gfxapi::TextureUsageFlags::RenderTarget | gfxapi::TextureUsageFlags::Sampled, const char* debugName = nullptr) {
        auto texture{ rtvCache.Get(frameNumber, extent, format, usage) };
#ifdef _DEBUG
        if (debugName) {
            device.SetDebugName(texture, debugName);
        }
#endif
        return texture;
    }

    // Samplers.
    gfxapi::SamplerHandleUnique samplerClampLinearLinear;
    gfxapi::SamplerHandleUnique samplerClampBorderBlackNearest;
    gfxapi::SamplerHandleUnique samplerClampBorderWhiteNearest;
    gfxapi::SamplerHandleUnique samplerClampNearestNearest;
    gfxapi::SamplerHandleUnique samplerWrapLinearNearest;

    // Texture.
    gfxapi::TextureHandleUnique textureWhite;
    gfxapi::TextureHandleUnique textureBlack;
    gfxapi::TextureHandleUnique textureNull;
    gfxapi::TextureHandleUnique textureCubeBlack;

    // Basic geometry.
    gfxapi::BufferHandleUnique skyBoxVertexBuffer;
    gfxapi::BufferHandleUnique skyBoxIndexBuffer;
    u32 skyBoxIndexCount{};

    // Framebuffers.
    gfxapi::FramebufferCache framebufferCache;
    gfxapi::RenderTargetCache rtvCache;
    gfxapi::RenderTargetCache shadowMapCache; // Keep separate shadow map render targets.
    gfxapi::Extent2D shadowMapResolution;

    // Pipelines
    gfxapi::ComputePipelineHandleUnique animationCSO;
    gfxapi::ComputePipelineHandleUnique genFrustumsCSO;

    // Passes
    UniquePtr<ShadowPass> shadowPass;
    UniquePtr<DepthPrePass> depthPrePass;
    UniquePtr<ForwardPass> forwardPass;
    UniquePtr<LightCullingPass> lightCullingPass;
    UniquePtr<SsaoPass> aoPass;
    // Postprocess passes
    UniquePtr<TonemappingPass> tonemappingPass;

    // TODO:
    ShaderVariants shaderVariants;
    u32 SHADER_DEPTH_PASS_MASK{};
    u32 SHADER_INSTANCED_MASK{};
    u32 SHADER_OPACITY_MASK{};
    u32 SHADER_MASKED_MASK{};

    //RenderThread renderThread;

private:
    gfxapi::RenderPassHandleUnique CreateDepthRenderPass();
    gfxapi::RenderPassHandleUnique CreateForwardRenderPass(gfxapi::Format format);
    gfxapi::RenderPassHandleUnique CreatePostProcessRenderPass(gfxapi::Format format);

    std::map<RenderPass, gfxapi::RenderPassHandleUnique> renderpasses;
};

} // namespace ugine
