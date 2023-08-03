#include "Shader.h"

#include <ugine/Log.h>

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/asset/SerializedShader.h>

#include <ugine/engine/shaders/Shader_Material.h>

namespace ugine {

u8 ShaderVariants::GetVariantIndex(StringView name) {
    auto index{ variants_.FindIf([&](const auto& s) { return s == name; }) };
    if (index >= 0) {
        return u8(index);
    }

    variants_.PushBack(name.Data());
    return u8(variants_.Size() - 1);
}

Shader::Shader(ResourceManager& manager, const ResourceID& id)
    : Resource{ manager, TYPE, id } {
}

bool Shader::HandleLoad(Span<const u8> data) {
    SerializedShader serialized{};
    if (!LoadShader(data, serialized)) {
        return false;
    }

    UGINE_DEBUG("Creating shader '{}'...", serialized.name);

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    name_ = serialized.name.c_str();
    category_ = serialized.category.c_str();

    variantMask_ = 0;
    constantMask_ = 0;

    for (const auto& define : serialized.defines) {
        const auto index{ state->shaderVariants.GetVariantIndex(define.c_str()) };

        constantMask_ |= UGINE_BIT(index);
    }

    for (const auto& variant : serialized.variants) {
        u32 variantMask{};

        for (const auto& define : variant.defines) {
            const auto defineIndex{ state->shaderVariants.GetVariantIndex(Span<const char>(define.c_str(), define.size())) };
            variantMask |= UGINE_BIT(defineIndex);
        }

        ShaderVariant& shaderVariant{ variants_[variantMask] };

        // Vertex input layout.
        for (const auto& attribute : variant.vertexAttributes) {
            shaderVariant.vertexAttributes.EmplaceBack(attribute.location, attribute.name.c_str());
        }

        // Params.
        for (const auto& [stage, params] : variant.stages) {
            for (const auto& [ds, dsParams] : params.datasets) {
                // TODO: Material params only.
                if (ds != DATASET_MATERIAL) {
                    continue;
                }

                shaderVariant.uniformSize = std::max(shaderVariant.uniformSize, dsParams.uniformSize);

                for (const auto& uniformVal : dsParams.params) {
                    shaderVariant.params.PushBack(ShaderParamDescriptor{
                        .binding = uniformVal.binding,
                        .offset = uniformVal.offset,
                        .size = uniformVal.size,
                        .type = uniformVal.type,
                        .name = uniformVal.name.c_str(),
                        .id = StringID{ uniformVal.name.c_str() },
                    });

                    params_[String{ uniformVal.name.c_str() }] = ShaderParam{
                        .type = uniformVal.type,
                        .name = uniformVal.name.c_str(),
                        .id = StringID{ uniformVal.name.c_str() },
                    };
                }
            }
        }

        // Compiled stages.
        for (const auto& [stage, serializedStage] : variant.stages) {
            auto& compiled{ shaderVariant.shaders[stage] };

            compiled.binary = Vector<u8>{ serializedStage.compiled.data(), serializedStage.compiled.size(), Manager().GetAllocator() };
            compiled.entry = String{ serializedStage.entry.c_str(), Manager().GetAllocator() };
        }

        variantMask_ |= variantMask;
    }

    // Default values.
    for (const auto& [value, key] : serialized.params) {
        auto& param{ defaultValues_[StringID{ value.c_str() }] };
        param.type = UniformValue::Type(key.type);
        memcpy(param.value.data(), key.value.data(), param.Size());
    }

    return true;
}

const UniformValue* Shader::DefaultShaderValue(const StringID& name) const {
    const auto it{ defaultValues_.find(name) };
    return it == defaultValues_.end() ? nullptr : &it->second;
}

bool Shader::HandleUnload() {
    variantMask_ = 0;
    variants_.clear();

    return true;
}

} // namespace ugine