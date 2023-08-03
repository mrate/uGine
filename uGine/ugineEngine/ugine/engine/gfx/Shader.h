#pragma once

#include <gfxapi/Types.h>

#include <ugine/String.h>

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/gfx/Uniform.h>

namespace ugine {

struct ShaderParamDescriptor {
    u32 binding{};
    u32 offset{};
    u32 size{};
    UniformValue::Type type{};
    String name{};
    StringID id{};
};

class ShaderVariants {
public:
    u8 GetVariantIndex(StringView name);

    const Vector<String>& Get() const { return variants_; }

private:
    Vector<String> variants_;
};

class Shader final : public Resource {
public:
    struct VertexAttribute {
        u32 location;
        String name;
    };

    struct ShaderBinary {
        Vector<u8> binary;
        String entry;
    };

    struct ShaderVariant {
        std::map<gfxapi::ShaderStage, ShaderBinary> shaders;
        Vector<VertexAttribute> vertexAttributes;

        // Merged params & uniform size for all stages.
        Vector<ShaderParamDescriptor> params;
        u32 uniformSize;
    };

    struct ShaderParam {
        UniformValue::Type type{};
        String name{};
        StringID id{};
    };

    inline static const ResourceType TYPE{ "Shader" };

    Shader(ResourceManager& manager, const ResourceID& id);
    ~Shader() { Unload(); }

    u32 VariantMask() const { return variantMask_; }
    u32 ConstantMask() const { return constantMask_; }

    const String& Name() const { return name_; }
    const String& Category() const { return category_; }

    Span<const ShaderParamDescriptor> Params(u32 variant) const {
        const auto it{ variants_.find(variant & variantMask_) };
        if (it != variants_.end()) {
            return it->second.params.ToSpan();
        }
        return {};
    }

    const std::unordered_map<u32, ShaderVariant>& Variants() const { return variants_; }

    const ShaderVariant& Variant(u32 variant) const { return variants_.at(VariantMask(variant)); }
    u32 VariantMask(u32 variant) const { return (variant | constantMask_) & variantMask_; }

    const std::map<String, ShaderParam>& Params() const { return params_; }
    const UniformValue* DefaultShaderValue(const StringID& name) const;

private:
    bool HandleLoad(Span<const u8> data) override;
    bool HandleUnload() override;

    u32 variantMask_{};  // Mask of all shader variants.
    u32 constantMask_{}; // Mask of #defines

    std::unordered_map<u32, ShaderVariant> variants_;

    // Merged parameters across all stages and variants.
    std::map<String, ShaderParam> params_;

    std::unordered_map<StringID, UniformValue> defaultValues_;

    String name_;
    String category_;
};

} // namespace ugine