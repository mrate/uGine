#include "MaterialCreator.h"

#include <MaterialBuilder.h>
#include <ugine/StringUtils.h>

#include <fstream>

namespace ugine::ed {

MaterialCreator::MaterialCreator(StringView name)
    : name_{ name } {
}

void MaterialCreator::AddTexture(SerializedTextureMapping mapping) {
    switch (mapping) {
    case SerializedTextureMapping::DiffuseMap: hasAlbedo_ = true; break;
    case SerializedTextureMapping::NormalMap: hasNormal_ = true; break;
    case SerializedTextureMapping::EmissiveMap: hasEmissive_ = true; break;
    default: break;
    }
}

void MaterialCreator::SetColor(SerializedColorMapping mapping, const glm::vec4& color) {
    switch (mapping) {
    case SerializedColorMapping::DiffuseColor: diffuseColor_ = color; break;
    case SerializedColorMapping::EmissiveColor: emissiveColor_ = color; break;
    default: break;
    }
}

Path MaterialCreator::Build(const Path& targetDir) {
    using namespace tools;

    std::stringstream shader;

    int bindings{};
    if (hasAlbedo_) {
        const auto binding{ ++bindings };
        shader << std::format("IMAGE_SAMPLER BINDING(DATASET_MATERIAL, {}) Texture2D albedoTexture;\n", binding);
        shader << std::format("IMAGE_SAMPLER BINDING(DATASET_MATERIAL, {}) SamplerState albedoTextureSampler;\n", binding);
        bindingToMapping_[bindings] = SerializedTextureMapping::DiffuseMap;
    }

    if (hasNormal_) {
        const auto binding{ ++bindings };
        shader << std::format("IMAGE_SAMPLER BINDING(DATASET_MATERIAL, {}) Texture2D normalTexture;\n", binding);
        shader << std::format("IMAGE_SAMPLER BINDING(DATASET_MATERIAL, {}) SamplerState normalTextureSampler;\n", binding);
        bindingToMapping_[bindings] = SerializedTextureMapping::NormalMap;
    }

    if (hasEmissive_) {
        const auto binding{ ++bindings };
        shader << std::format("IMAGE_SAMPLER BINDING(DATASET_MATERIAL, {}) Texture2D emissiveTexture;\n", binding);
        shader << std::format("IMAGE_SAMPLER BINDING(DATASET_MATERIAL, {}) SamplerState emissiveTextureSampler;\n", binding);
        bindingToMapping_[bindings] = SerializedTextureMapping::EmissiveMap;
    }

    shader << R"shader(
float3 material_vertex_offset() { return 0.0f; }

void material_bp_fragment(inout MaterialBpFragment fragment, float2 uv) {
)shader";

    if (hasAlbedo_) {
        if (diffuseColor_) {
            shader << std::format("fragment.albedo = float3({}, {}, {}) * albedoTexture.Sample(albedoTextureSampler, uv).rgb;\n", diffuseColor_->x,
                diffuseColor_->y, diffuseColor_->z);
        } else {
            shader << "fragment.albedo = albedoTexture.Sample(albedoTexture, uv).rgb;\n";
        }
    }
    if (hasNormal_) {
        shader << "fragment.normal = normalize(normalTexture.Sample(normalTextureSampler, uv).rgb * 2 - 1);\n";
        shader << "fragment.hasNormal = true;\n";
    }
    if (hasEmissive_) {
        if (emissiveColor_) {
            shader << std::format("fragment.emissive = float3({}, {}, {}) * emissiveTexture.Sample(emissiveTextureSampler, uv).rgb;\n", emissiveColor_->x,
                emissiveColor_->y, emissiveColor_->z);
        } else {
            shader << "fragment.emissive = emissiveTexture.Sample(emissiveTextureSampler, uv).rgb;\n";
        }
    }

    // TODO:
    //#ifdef MATERIAL_MASK
    //    fragment.mask = 1;
    //#endif

    shader << "}\n";

    MaterialDescription description{
        .name = name_.Data(),
        .source = shader.str(),
        .domain = MaterialDomain::Surface,
        .shading = MaterialShadingModel::Lit,
        .blendMode = MaterialBlendMode::Opaque, // TODO:
        .doubleSided = false,
        .wireframe = false,
    };

    MaterialBuilder builder{ name_.Data(), description };

    std::filesystem::path outputMaterial{ (targetDir / std::format("{}.mat", name_).c_str()).Data() }; // TODO: [PATH]

    {
        std::ofstream out{ outputMaterial };
        out << builder.Descriptor();
    }

    {
        for (const auto& [shader, source] : builder.Shaders()) {
            std::filesystem::path outputShader{ (targetDir / shader).Data() }; // TODO: [PATH]
            std::ofstream out{ outputShader };
            out << source;
        }
    }

    return Path{ outputMaterial.string() };
}

} // namespace ugine::ed