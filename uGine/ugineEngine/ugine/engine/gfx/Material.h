#pragma once

#include "Pipeline.h"

#include <gfxapi/Handle.h>
#include <gfxapi/Types.h>

#include <ugine/String.h>

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/gfx/Texture.h>

#include <glm/glm.hpp>

#include <array>
#include <map>
#include <string_view>

namespace ugine {

class GraphicsState;

#pragma pack(push, 1)
struct MaterialVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
};

// Instance 4x3 MVP.
struct MaterialVertexInstance {
    glm::vec4 instance0{ 1, 0, 0, 0 };
    glm::vec4 instance1{ 0, 1, 0, 0 };
    glm::vec4 instance2{ 0, 0, 1, 0 };
};
#pragma pack(pop)

glm::mat4 ToMatrix(const MaterialVertexInstance& instance);
MaterialVertexInstance MaterialVertexInstanceFromMatrix(const glm::mat4& matrix);

enum class MaterialBlending : u8 {
    Opaque = 0,
    Masked,
    Additive,
    Translucent,
};

struct MaterialPipeline {
    bool doubleSided{};
    bool depthWrite{ true };
    bool depthTest{ true };
    bool wireframe{};
    MaterialBlending blending{ MaterialBlending::Opaque };
    // TODO: Material domain.
};

struct SerializedMaterial;

class Material final : public Resource {
public:
    inline static const ResourceType TYPE{ "Material" };

    static ResourceHandle<Material> InstanceOrigin(ResourceHandle<Material> material) { return material->IsInstance() ? material->instanceOrigin_ : material; }

    Material(ResourceManager& resourceManager, const ResourceID& id)
        : Resource{ resourceManager, TYPE, id } {}

    ~Material() { Unload(); }

    void Serialize(SerializedMaterial& serialized) const;

    UGINE_FORCE_INLINE bool IsTransparent() const {
        return pipelineDesc_.blending != MaterialBlending::Opaque && pipelineDesc_.blending != MaterialBlending::Masked;
    }
    UGINE_FORCE_INLINE bool IsInstance() const { return isInstance_; }

    UGINE_FORCE_INLINE bool HasParam(const StringID& name) const { return params_.contains(name); }

    template <typename T> T GetParam(const StringID& name) const {
        auto it{ params_.find(name) };
        if (it != params_.end()) {
            return it->second.Get<T>();
        }

        if (IsInstance()) {
            auto it{ instanceOrigin_->params_.find(name) };
            if (it != instanceOrigin_->params_.end()) {
                return it->second.Get<T>();
            }
        }

        if (it == params_.end() && shader_) {
            const auto defaultValue{ shader_->DefaultShaderValue(name) };
            if (defaultValue) {
                return defaultValue->Get<T>();
            }
        }

        return T{};
    }

    template <typename T> void SetParam(const StringID& name, UniformValue::Type type, const T& value) {
        auto& param{ params_[name] };

        if (param.type == UniformValue::Type::TextureID) {
            RemoveTexture(param.Get<ResourceID>());
        }

        param = UniformValue{
            .name = name,
            .type = type,
        };

        UGINE_ASSERT(sizeof(value) >= param.Size());
        memcpy(&param.value, &value, param.Size());

        if (param.type == UniformValue::Type::TextureID) {
            AddTexture(param.Get<ResourceID>());
        }

        InvalidateParams();
    }

    UGINE_FORCE_INLINE void ResetParam(const StringID& name) {
        params_.erase(name);
        InvalidateParams();
    }

    UGINE_FORCE_INLINE void SetFloat(const StringID& name, float value) { SetParam(name, UniformValue::Type::Float, value); }
    UGINE_FORCE_INLINE void SetFloat2(const StringID& name, const glm::vec2& vec) { SetParam(name, UniformValue::Type::Float2, vec); }
    UGINE_FORCE_INLINE void SetFloat3(const StringID& name, const glm::vec3& vec) { SetParam(name, UniformValue::Type::Float3, vec); }
    UGINE_FORCE_INLINE void SetFloat4(const StringID& name, const glm::vec4& vec) { SetParam(name, UniformValue::Type::Float4, vec); }
    UGINE_FORCE_INLINE void SetInt(const StringID& name, i32 value) { SetParam(name, UniformValue::Type::Int, value); }
    UGINE_FORCE_INLINE void SetInt2(const StringID& name, const glm::ivec2& vec) { SetParam(name, UniformValue::Type::Int2, vec); }
    UGINE_FORCE_INLINE void SetInt3(const StringID& name, const glm::ivec3& vec) { SetParam(name, UniformValue::Type::Int3, vec); }
    UGINE_FORCE_INLINE void SetInt4(const StringID& name, const glm::ivec4& vec) { SetParam(name, UniformValue::Type::Int4, vec); }
    UGINE_FORCE_INLINE void SetTexture(const StringID& name, const ResourceID& id) { SetParam(name, UniformValue::Type::TextureID, id); }

    gfxapi::BufferHandle GetUniform(u32 variantMask) const {
        const auto it{ paramsBuffer_.find(GetPipeline().VariantMask(variantMask)) };
        if (it == paramsBuffer_.end()) {
            return gfxapi::BufferHandle{};
        }

        const auto& params{ it->second };
        return *params.buffer[params.index];
    }

    UGINE_FORCE_INLINE ResourceHandle<Shader> GetShader() { return isInstance_ ? instanceOrigin_->GetShader() : shader_; }
    void SetShader(ResourceHandle<Shader> shader);

    UGINE_FORCE_INLINE const MaterialPipeline& GetMaterialPipeline() const { return pipelineDesc_; }
    void SetMaterialPipeline(const MaterialPipeline& desc);

    // Rendering API
    UGINE_FORCE_INLINE gfxapi::GraphicsPipelineHandle GetPipeline(u32 variantMask) const {
        return IsInstance() ? instanceOrigin_->GetPipeline(variantMask) : pipeline_.GetPipeline(variantMask);
    }

    void Prepare(GraphicsState& state, u32 variantMask);

    bool HasVariant(uint32_t variant) const;

private:
    // Resource::*
    bool HandleLoad(Span<const u8> data) override;
    bool HandleUnload() override;
    void HandleDependenciesReady() override;
    void HandleDependencyChanged(const ResourceID& id, ResourceState state) override;

    // Material::*
    void InitShader();
    void DestroyShader();

    void InitPipeline();
    void DestroyPipeline();

    bool InitMaterial(const SerializedMaterial& serialized);
    bool InitMaterialInstance(const SerializedMaterial& serialized);

    void InitInstance(ResourceHandle<Material> origin);

    void InitParams();
    void DestroyParams();

    void InvalidateParams();

    void AddTexture(const ResourceID& texture);
    void RemoveTexture(const ResourceID& texture);
    void RemoveTextures();

    Pipeline& GetPipeline() { return isInstance_ ? instanceOrigin_->GetPipeline() : pipeline_; }
    const Pipeline& GetPipeline() const { return isInstance_ ? instanceOrigin_->GetPipeline() : pipeline_; }

    ResourceHandle<Shader> shader_;
    bool shaderInitialized_{};

    MaterialPipeline pipelineDesc_;
    Pipeline pipeline_;

    bool isInstance_{};
    std::unordered_map<StringID, UniformValue> params_;

    // Instance.
    bool instanceInitialized_{};
    ResourceHandle<Material> instanceOrigin_;

    // Bindings.
    struct VariantParamBuffer {
        bool needsUpdate{};
        Vector<gfxapi::BufferHandleUnique> buffer;
        u32 index{};
    };

    std::unordered_map<u32, VariantParamBuffer> paramsBuffer_;

    std::unordered_map<ResourceID, ResourceHandle<Texture>> textures_;
}; // namespace ugine

} // namespace ugine