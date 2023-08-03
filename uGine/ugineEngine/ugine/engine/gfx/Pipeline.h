#pragma once

#include <ugine/engine/gfx/Shader.h>

#include <ugine/Vector.h>

#include <gfxapi/Types.h>

namespace ugine {

class GraphicsState;

struct SerializedMaterial;
struct MaterialPipeline;

class Pipeline {
public:
    Pipeline() = default;
    Pipeline(GraphicsState& state, const MaterialPipeline& pipeline, ResourceHandle<Shader> shader);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&& other) noexcept
        : materialMask_{ other.materialMask_ }
        , shader_{ other.shader_ } {
        other.shader_ = {};
        std::swap(pipelines_, other.pipelines_);
    }

    Pipeline& operator=(Pipeline&& other) noexcept {
        UGINE_ASSERT(pipelines_.empty());

        materialMask_ = other.materialMask_;
        shader_ = other.shader_;
        other.shader_ = {};
        std::swap(pipelines_, other.pipelines_);
        other.pipelines_.clear();

        return *this;
    }

    void Destroy(GraphicsState& state);

    gfxapi::GraphicsPipelineHandle GetPipeline(u32 variantMask) const;
    u32 VariantMask(u32 variant) const { return shader_->VariantMask(variant | materialMask_); }

    bool Empty() const { return pipelines_.empty(); }

private:
    u32 materialMask_{};
    ResourceHandle<Shader> shader_;
    std::unordered_map<u32, gfxapi::GraphicsPipelineHandle> pipelines_;
};

} // namespace ugine