#include "GeometryPass.h"

#include <ugine/engine/shaders/Shader_Material.h>

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/RenderContext.h>

#include <gfxapi/Device.h>
#include <ugine/Profile.h>

#include <span>

using namespace ugine::gfxapi;

namespace ugine {

void GeometryPass::RenderGeometry(gfxapi::CommandList& cmd, RenderContext& context, bool depth, bool transparent) {
    UGINE_GPU_EVENT(cmd, label, "RenderGeometry");

    gfxapi::GraphicsPipelineHandle boundPipeline{};
    gfxapi::BufferHandle boundVertexBuffer{};
    gfxapi::BufferHandle boundInstanceBuffer{};
    gfxapi::BufferHandle boundIndexBuffer{};

    for (size_t i{}; i < context.draws.Size(); ++i) {
        auto& draw{ context.draws[i] };
        const auto isTransparent{ (draw.flags & Draw::FLAG_TRANSPARENT) != 0 };

        UGINE_ASSERT(draw.instanceCount > 0);

        if (isTransparent != transparent) {
            continue;
        }

        auto pipeline{ depth ? draw.depthPipeline : draw.pipeline };
        if (!pipeline) {
            continue;
        }

        PROFILE_EVENT_NC("BindMaterial", COLOR_PROFILE_GRAPHICS);

        if (pipeline != boundPipeline) {
            PROFILE_EVENT_NC("BindPipeline", COLOR_PROFILE_GRAPHICS);

            boundPipeline = pipeline;
            cmd.BindPipeline(pipeline);

            cmd.BindUniform(DATASET_GLOBAL, SLOT_GLOBAL, context.gpuGlobalCB);
            cmd.BindUniform(DATASET_GLOBAL, SLOT_CAMERA, context.gpuCameraCB);

            if (!depth) {
                cmd.BindStorage(DATASET_GLOBAL, SLOT_LIGHTS, context.gpuLightListSB);
                cmd.BindStorage(DATASET_GLOBAL, SLOT_SHADOWS, context.gpuShadowsSB);

                if (transparent) {
                    cmd.BindImage(DATASET_GLOBAL, SLOT_LIGHT_GRID, context.gpuTransparentLightGrid);
                    cmd.BindStorage(DATASET_GLOBAL, SLOT_LIGHT_INDEX, context.gpuTransparentLightIndexList);
                } else {
                    cmd.BindImage(DATASET_GLOBAL, SLOT_LIGHT_GRID, context.gpuOpaqueLightGrid);
                    cmd.BindStorage(DATASET_GLOBAL, SLOT_LIGHT_INDEX, context.gpuOpaqueLightIndexList);
                }
            }

            const auto uniform{ depth ? draw.depthUniform : draw.uniform };
            if (uniform) {
                cmd.BindUniform(DATASET_MATERIAL, 0, uniform);
            }
        }

        if (draw.vertexBuffer != boundVertexBuffer || draw.instanceBuffer != boundInstanceBuffer) {
            PROFILE_EVENT_NC("BindVB", COLOR_PROFILE_GRAPHICS);

            boundVertexBuffer = draw.vertexBuffer;
            boundInstanceBuffer = draw.instanceBuffer;

            if (draw.instanceBuffer) {
                cmd.BindVertexBuffers(draw.vertexBuffer, draw.instanceBuffer, 0, 0);
            } else {
                cmd.BindVertexBuffer(draw.vertexBuffer, 0);
            }
        }

        if (draw.indexBuffer != boundIndexBuffer) {
            PROFILE_EVENT_NC("BindIB", COLOR_PROFILE_GRAPHICS);

            boundIndexBuffer = draw.indexBuffer;
            cmd.BindIndexBuffer(draw.indexBuffer, 0, draw.indexType);
        }

        cmd.SetStencilWriteMask(StencilFaceFlags::FrontAndBack, draw.stencil);

        auto buffer{ cmd.AllocateGPU(sizeof(shaders::Draw)) };
        *buffer.As<shaders::Draw>() = shaders::Draw{
            .model = draw.model,
            .normal = draw.normal,
        };
        cmd.BindUniform(DATASET_DRAW, 0, buffer);

        cmd.DrawIndexed(draw.indexCount, draw.instanceCount, draw.indexOffset, draw.vertexOffset, 0);
        ++context.drawCallsCounter;
    }
}
} // namespace ugine