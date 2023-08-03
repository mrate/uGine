#pragma once

#include <gfxapi/CommandList.h>
#include <gfxapi/Types.h>

#include <ugine/Vector.h>

namespace ugine {

class GraphicsState;

class DebugRenderer {
public:
    UGINE_MOVABLE_ONLY(DebugRenderer);

    DebugRenderer() = default;
    DebugRenderer(GraphicsState& state, gfxapi::RenderPassHandle renderPass, IAllocator& allocator = IAllocator::Default());

    //void RenderMesh(GPUCommandList command, const Mesh& mesh, const glm::vec4& color, const glm::mat4& mvp);
    void RenderCube(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& mvp);
    void RenderIcosphere(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& mvp);
    void RenderCone(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& mvp);
    void RenderCylinder(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& mvp);
    void RenderCapsule(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& mvp);
    //void RenderAABB(gfxapi::CommandList& command, const math::AABB& aabb, const glm::vec4& color, const glm::mat4& mvp);
    void RenderFrustum(gfxapi::CommandList& command, f32 vFov, f32 aspectRatio, f32 zNear, f32 zFar, const glm::vec4& color, const glm::mat4& mvp);
    void RenderDirection(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& mvp);
    void RenderLine(gfxapi::CommandList& command, const glm::vec4& color, const glm::mat4& viewProj, const glm::vec3& from, const glm::vec3& to);

    // TODO: New interface
    void Clear(IAllocator& allocator = IAllocator::Default());
    void AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    void AddCube(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    void AddTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& color);
    void AddSphere(const glm::vec3& center, f32 radius, const glm::vec3& color);
    void AddCircle(const glm::vec3& position, f32 radius, const glm::vec3& color);
    void AddCuboid(glm::vec3 vertices[8], const glm::vec3& color);

    void Render(gfxapi::CommandList& command, const glm::mat4 viewProj);

private:
    enum Pipelines {
        PipelineFrustum = 0,
        PipelineCircle,
        PipelineCube,
        PipelineIcosphere,
        PipelineCone,
        PipelineCylinder,
        PipelineCapsule,
        PipelineDir,
        PipelineLine,
        PIPELINES_COUNT,
    };

    struct alignas(16) Frustum {
        glm::mat4 mvp{};
        f32 vfovTanHalf{};
        f32 hfovTanHalf{};
        f32 zNear{};
        f32 zFar{};
    };

    struct Line {
        glm::mat4 viewProj{};
        alignas(16) glm::vec3 from{};
        alignas(16) glm::vec3 to{};
    };

    struct Lines {
        glm::mat4 viewProj{};
    };

    std::array<gfxapi::GraphicsPipelineHandleUnique, PIPELINES_COUNT> pipelines_;

    gfxapi::GraphicsPipelineHandleUnique debugPipeline_;

    struct DebugVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    struct DebugSphere {
        glm::vec3 center;
        f32 radius{};
        glm::vec4 color{};
    };

    struct DebugCircle {
        glm::vec3 center;
        f32 radius{};
        glm::vec4 color{};
    };

    Vector<DebugVertex> lines_;
    Vector<DebugSphere> spheres_;
    Vector<DebugCircle> circles_;

    gfxapi::BufferHandleUnique debugVertices_;
    u64 debugVerticesSize_{};
};

} // namespace ugine
