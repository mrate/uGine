#include "DebugRenderer.h"

#include <gfxapi/Device.h>
#include <ugine/engine/gfx/GraphicsState.h>

#include <glm/gtx/transform.hpp>

#include <ugineTools/Embed.h>

#include <shaders/gizmoCapsule_vs_hlsl.h>
#include <shaders/gizmoCircle_vs_hlsl.h>
#include <shaders/gizmoCone_vs_hlsl.h>
#include <shaders/gizmoCube_vs_hlsl.h>
#include <shaders/gizmoCylinder_vs_hlsl.h>
#include <shaders/gizmoDir_vs_hlsl.h>
#include <shaders/gizmoFrustum_vs_hlsl.h>
#include <shaders/gizmoIcosphere_vs_hlsl.h>
#include <shaders/gizmoLine_vs_hlsl.h>
#include <shaders/gizmo_fs_hlsl.h>

#include <shaders/debug_fs_hlsl.h>
#include <shaders/debug_vs_hlsl.h>

using namespace ugine::gfxapi;

namespace ugine {

DebugRenderer::DebugRenderer(GraphicsState& state, gfxapi::RenderPassHandle renderPass, IAllocator& allocator)
    : lines_{ allocator } {

    const std::pair<CompiledShader, PrimitiveTopology> ps[PIPELINES_COUNT] = {
        {
            {
                // Frustum
                .name = "GizmoFrustumVS",
                .entryPoint = "main",
                .data = gizmoFrustum_vs.data,
                .size = gizmoFrustum_vs.size,
            },
            PrimitiveTopology::LineList,
        },
        {
            {
                // Circle
                .name = "GizmoCircleVS",
                .entryPoint = "main",
                .data = gizmoCircle_vs.data,
                .size = gizmoCircle_vs.size,
            },
            PrimitiveTopology::LineStrip,
        },
        {
            {
                // Cube
                .name = "GizmoCubeVS",
                .entryPoint = "main",
                .data = gizmoCube_vs.data,
                .size = gizmoCube_vs.size,
            },
            PrimitiveTopology::LineList,
        },
        {
            {
                // Icosphere
                .name = "GizmoIcosphereVS",
                .entryPoint = "main",
                .data = gizmoIcosphere_vs.data,
                .size = gizmoIcosphere_vs.size,
            },
            PrimitiveTopology::TriangleList,
        },
        {
            {
                // Cone
                .name = "GizmoConeVS",
                .entryPoint = "main",
                .data = gizmoCone_vs.data,
                .size = gizmoCone_vs.size,
            },
            PrimitiveTopology::TriangleList,
        },
        {
            {
                // Cylinder
                .name = "GizmoCylinderVS",
                .entryPoint = "main",
                .data = gizmoCylinder_vs.data,
                .size = gizmoCylinder_vs.size,
            },
            PrimitiveTopology::TriangleList,
        },
        {
            {
                // Capsule
                .name = "GizmoCapsuleVS",
                .entryPoint = "main",
                .data = gizmoCapsule_vs.data,
                .size = gizmoCapsule_vs.size,
            },
            PrimitiveTopology::TriangleList,
        },
        {
            {
                // Dir
                .name = "GizmoDirVS",
                .entryPoint = "main",
                .data = gizmoDir_vs.data,
                .size = gizmoDir_vs.size,
            },
            PrimitiveTopology::LineList,
        },
        {
            {
                // Line
                .name = "GizmoLineVS",
                .entryPoint = "main",
                .data = gizmoLine_vs.data,
                .size = gizmoLine_vs.size,
            },
            PrimitiveTopology::LineList,
        },
    };

    const CompiledShader fs{
        .name = "GizmoFS",
        .entryPoint = "main",
        .data = gizmo_fs.data,
        .size = gizmo_fs.size,
    };

    for (int i{}; i < PIPELINES_COUNT; ++i) {
        pipelines_[i] = state.device.CreateGraphicsPipelineUnique(GraphicsPipelineDesc { 
            .name = "DebugRenderer", 
            .vertexShader = ps[i].first, 
            .fragmentShader = fs,
            .rasterizerState = {
                .fillMode = FillMode::Line,
            },
            .depthStencilState = {
                .depthTestEnable = true,
                .depthWriteEnable = false,
            },
            .inputAssembly = {
                .primitiveTopology = ps[i].second,
            },
            .renderPass = renderPass,
        });
    }

    std::array<VertexAttribute, 2> debugVertexAttributes = {
        VertexAttribute{
            0,
            0,
            offsetof(DebugVertex, position),
            gfxapi::Format::R32G32B32_Float,
            gfxapi::InputSlotClass::PerVertex,
        },
        VertexAttribute{
            0,
            1,
            offsetof(DebugVertex, color),
            gfxapi::Format::R32G32B32A32_Float,
            gfxapi::InputSlotClass::PerVertex,
        },
    };

    debugPipeline_ = state.device.CreateGraphicsPipelineUnique(GraphicsPipelineDesc { 
            .name = "DebugRenderer", 
            .vertexShader = CompiledShader {
                .name = "DebugVS",
                .entryPoint = "main",
                .data = debug_vs.data,
                .size = debug_vs.size,
            },
            .fragmentShader = CompiledShader {
                .name = "DebugFS",
                .entryPoint = "main",
                .data = debug_fs.data,
                .size = debug_fs.size,
            },
            .rasterizerState = {
                .fillMode = FillMode::Line,
            },  
            .depthStencilState = {
                .depthTestEnable = true,
                .depthWriteEnable = false,
            },
            .inputAssembly = {
                .primitiveTopology = PrimitiveTopology::LineList,
                .vertexAttributes = debugVertexAttributes.data(),
                .vertexAttributesCount = u32(debugVertexAttributes.size()),
                .vertexBindingsCount = 1,
                .vertexBindings = {
                    VertexBindings {
                        .dataStride = sizeof(DebugVertex),
                        .slot = InputSlotClass::PerVertex,
                    }
                },
            },
            .renderPass = renderPass,
        });
}

//void DebugRenderer::RenderMesh(CommandList&, const Mesh& mesh, const glm::vec4& color, const glm::mat4& mvp) {
//    device.BindPipeline(command, &pipelines_[PipelineMesh].pipeline);
//    device.PushConstants(command, GPUPushConstant::Data(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, mvp));
//    device.PushConstants(command, GPUPushConstant::Data(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, color, 112));
//    device.BindVertexIndexBuffer(command, *mesh.VertexBuffer());
//
//    const auto& drawCall{ mesh.DrawCall(0) };
//    device.DrawIndexed(command, drawCall.indexCount, drawCall.instanceCount, drawCall.indexStart, drawCall.vertexOffset, drawCall.firstInstance);
//}

void DebugRenderer::RenderCube(CommandList& cmd, const glm::vec4& color, const glm::mat4& mvp) {
    cmd.BindPipeline(*pipelines_[PipelineCube]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, mvp);
    cmd.Draw(24, 1, 0, 0);
}

void DebugRenderer::RenderIcosphere(gfxapi::CommandList& cmd, const glm::vec4& color, const glm::mat4& mvp) {
    cmd.BindPipeline(*pipelines_[PipelineIcosphere]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, mvp);
    cmd.Draw(240, 1, 0, 0);
}

void DebugRenderer::RenderCone(gfxapi::CommandList& cmd, const glm::vec4& color, const glm::mat4& mvp) {
    cmd.BindPipeline(*pipelines_[PipelineCone]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, mvp);
    cmd.Draw(192, 1, 0, 0);
}

void DebugRenderer::RenderCylinder(gfxapi::CommandList& cmd, const glm::vec4& color, const glm::mat4& mvp) {
    cmd.BindPipeline(*pipelines_[PipelineCylinder]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, mvp);
    cmd.Draw(384, 1, 0, 0);
}

void DebugRenderer::RenderCapsule(gfxapi::CommandList& cmd, const glm::vec4& color, const glm::mat4& mvp) {
    cmd.BindPipeline(*pipelines_[PipelineCapsule]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, mvp);
    cmd.Draw(2244, 1, 0, 0);
}

//void DebugRenderer::RenderAABB(CommandList&, const math::AABB& aabb, const glm::vec4& color, const glm::mat4& mvp) {
//    RenderCube(command, color, mvp * glm::translate(glm::mat4{ 1.0f }, aabb.CenterPoint()) * glm::scale(glm::mat4{ 1.0f }, aabb.Size()));
//}

void DebugRenderer::RenderFrustum(CommandList& cmd, f32 vFov, f32 aspectRatio, f32 zNear, f32 zFar, const glm::vec4& color, const glm::mat4& mvp) {
    const Frustum frustum{
        .mvp = mvp,
        .vfovTanHalf = tan(0.5f * glm::radians(vFov)),
        .hfovTanHalf = frustum.vfovTanHalf * aspectRatio,
        .zNear = zNear,
        .zFar = zFar,
    };

    cmd.BindPipeline(*pipelines_[PipelineFrustum]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, frustum);
    cmd.Draw(24, 1, 0, 0);
}

void DebugRenderer::RenderDirection(CommandList& cmd, const glm::vec4& color, const glm::mat4& mvp) {
    cmd.BindPipeline(*pipelines_[PipelineDir]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, mvp);
    cmd.Draw(8, 1, 0, 0);
}

void DebugRenderer::RenderLine(CommandList& cmd, const glm::vec4& color, const glm::mat4& viewProj, const glm::vec3& from, const glm::vec3& to) {
    const Line line{
        .viewProj = viewProj,
        .from = from,
        .to = to,
    };

    cmd.BindPipeline(*pipelines_[PipelineLine]);
    cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, color);
    cmd.PushConstants(ShaderStage::VertexShader, 16, line);
    cmd.Draw(2, 1, 0, 0);
}

void DebugRenderer::Clear(IAllocator& allocator) {
    lines_ = Vector<DebugVertex>(allocator);
    spheres_ = Vector<DebugSphere>(allocator);
    circles_ = Vector<DebugCircle>(allocator);
}

void DebugRenderer::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    lines_.PushBack(DebugVertex{ .position = from, .color = color });
    lines_.PushBack(DebugVertex{ .position = to, .color = color });
}

void DebugRenderer::AddCube(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    AddLine(glm::vec3{ from.x, from.y, from.z }, glm::vec3{ to.x, from.y, from.z }, color);
    AddLine(glm::vec3{ from.x, from.y, from.z }, glm::vec3{ from.x, to.y, from.z }, color);
    AddLine(glm::vec3{ to.x, to.y, from.z }, glm::vec3{ to.x, from.y, from.z }, color);
    AddLine(glm::vec3{ to.x, to.y, from.z }, glm::vec3{ from.x, to.y, from.z }, color);

    AddLine(glm::vec3{ from.x, from.y, to.z }, glm::vec3{ to.x, from.y, to.z }, color);
    AddLine(glm::vec3{ from.x, from.y, to.z }, glm::vec3{ from.x, to.y, to.z }, color);
    AddLine(glm::vec3{ to.x, to.y, to.z }, glm::vec3{ to.x, from.y, to.z }, color);
    AddLine(glm::vec3{ to.x, to.y, to.z }, glm::vec3{ from.x, to.y, to.z }, color);

    AddLine(glm::vec3{ from.x, to.y, from.z }, glm::vec3{ from.x, to.y, to.z }, color);
    AddLine(glm::vec3{ from.x, from.y, from.z }, glm::vec3{ from.x, from.y, to.z }, color);
    AddLine(glm::vec3{ to.x, from.y, from.z }, glm::vec3{ to.x, from.y, to.z }, color);
    AddLine(glm::vec3{ to.x, to.y, from.z }, glm::vec3{ to.x, to.y, to.z }, color);
}

void DebugRenderer::AddTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& color) {
    AddLine(v1, v2, color);
    AddLine(v1, v3, color);
    AddLine(v2, v3, color);
}

void DebugRenderer::AddSphere(const glm::vec3& center, f32 radius, const glm::vec3& color) {
    spheres_.PushBack(DebugSphere{ .center = center, .radius = radius, .color = glm::vec4{ color, 1.0f } });
}

void DebugRenderer::AddCircle(const glm::vec3& center, f32 radius, const glm::vec3& color) {
    circles_.PushBack(DebugCircle{ .center = center, .radius = radius, .color = glm::vec4{ color, 1.0f } });
}

void DebugRenderer::AddCuboid(glm::vec3 vertices[8], const glm::vec3& color) {
    AddLine(vertices[0], vertices[1], color);
    AddLine(vertices[1], vertices[2], color);
    AddLine(vertices[2], vertices[3], color);
    AddLine(vertices[3], vertices[0], color);

    AddLine(vertices[4], vertices[5], color);
    AddLine(vertices[5], vertices[6], color);
    AddLine(vertices[6], vertices[7], color);
    AddLine(vertices[7], vertices[4], color);

    AddLine(vertices[0], vertices[4], color);
    AddLine(vertices[1], vertices[5], color);
    AddLine(vertices[2], vertices[6], color);
    AddLine(vertices[3], vertices[7], color);
}

void DebugRenderer::Render(gfxapi::CommandList& cmd, const glm::mat4 viewProj) {
    if (!lines_.Empty()) {
        const auto debugSize{ sizeof(DebugVertex) * lines_.Size() };
        if (debugVerticesSize_ < debugSize) {
            using namespace ugine::gfxapi;

            debugVerticesSize_ = 2 * debugSize;
            debugVertices_ = cmd.GetDevice().CreateBufferUnique(
                BufferDesc{
                    .flags = BufferFlags::Vertex,
                    .size = debugVerticesSize_,
                    .cpuAccess = CpuAccessFlags::Write,
                },
                lines_.Data(), debugSize);
        } else {
            auto mem{ cmd.GetDevice().GetBufferMapped(*debugVertices_) };
            memcpy(mem, lines_.Data(), debugSize);
        }

        cmd.BindPipeline(*debugPipeline_);
        cmd.BindVertexBuffer(*debugVertices_, 0);
        cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, viewProj);
        cmd.Draw(u32(lines_.Size()), 1, 0, 0);
    }

    if (!spheres_.Empty()) {
        cmd.BindPipeline(*pipelines_[PipelineIcosphere]);

        for (const auto& sphere : spheres_) {
            const glm::mat4 mvp{ viewProj * glm::translate(glm::vec3{ sphere.center }) * glm::scale(glm::vec3{ sphere.radius }) };

            cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, sphere.color);
            cmd.PushConstants(ShaderStage::VertexShader, 16, mvp);
            cmd.Draw(240, 1, 0, 0);
        }
    }

    if (!circles_.Empty()) {
        cmd.BindPipeline(*pipelines_[PipelineCircle]);

        for (const auto& circle : circles_) {
            const glm::mat4 mvp{ viewProj * glm::translate(glm::vec3{ circle.center }) * glm::scale(glm::vec3{ circle.radius }) };

            cmd.PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, circle.color);
            for (int i{ 1 }; i <= 3; i >>= 1) {
                const glm::vec3 axis{ i & 0x1, (i & 0x2) >> 1, (i & 0x4) >> 2 };
                cmd.PushConstants(ShaderStage::VertexShader, 16, mvp * glm::rotate(glm::pi<float>(), axis));
                cmd.Draw(91, 1, 0, 0);
            }
        }
    }
}

} // namespace ugine
