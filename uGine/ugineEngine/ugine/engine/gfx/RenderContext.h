#pragma once

#include <ugine/engine/gfx/Material.h>
#include <ugine/engine/shaders/Shader_Types.h>

#include <ugine/Span.h>

#include <gfxapi/CommandList.h>
#include <gfxapi/Handle.h>
#include <gfxapi/Types.h>

#include <array>

namespace ugine {

class GraphicsState;
class Camera;

struct Draw {
    enum Flags : u32 {
        FLAG_INSTANCED = UGINE_BIT(0),
        FLAG_TRANSPARENT = UGINE_BIT(1),
    };

    u64 sortKey{};

    glm::mat4 model{};
    glm::mat4 normal{};

    u32 indexCount{};
    u32 indexOffset{};
    u32 vertexOffset{};
    u32 instanceCount{};

    gfxapi::BufferHandle vertexBuffer{};
    gfxapi::BufferHandle indexBuffer{};
    gfxapi::BufferHandle instanceBuffer{};
    gfxapi::IndexType indexType{ gfxapi::IndexType::Uint16 };

    gfxapi::GraphicsPipelineHandle depthPipeline{};
    gfxapi::BufferHandle depthUniform{};

    gfxapi::GraphicsPipelineHandle pipeline{};
    gfxapi::BufferHandle uniform{};

    u32 stencil{};
    u32 flags{};
};

struct RenderContext {
    GraphicsState* state{};

    gfxapi::Extent2D extent{};
    ColorRGBA clearColor{};
    f32 clearDepth{};
    u8 clearStencil{};

    // Render targets.
    gfxapi::TextureHandle renderTargetHDR;
    gfxapi::TextureHandle depthBuffer;
    gfxapi::TextureHandle aoTexture;

    // Postprocess.
    u32 activePostProcessTexture{};
    gfxapi::TextureHandle postprocessPingPong[2];

    // Light culling.
    gfxapi::TextureHandle gpuOpaqueLightGrid;
    gfxapi::TextureHandle gpuTransparentLightGrid;
    gfxapi::GpuAllocation gpuOpaqueLightIndexList;
    gfxapi::GpuAllocation gpuTransparentLightIndexList;

    // Global data.
    shaders::Global globalCB{};
    gfxapi::GpuAllocation gpuGlobalCB{};

    gfxapi::GpuAllocation gpuLightListSB{};
    gfxapi::GpuAllocation gpuShadowsSB{};

    shaders::Camera cameraCB{};
    gfxapi::GpuAllocation gpuCameraCB{};

    gfxapi::BufferHandle gpuLightCullFrustums;

    Span<const Draw> draws;

    // Stats.
    u32 drawCallsCounter{};
    u32 computeDispatches{};
};

} // namespace ugine