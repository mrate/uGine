#include <MaterialBuilder.h>

#include <nlohmann/json.hpp>

#include <sstream>

namespace ugine::tools {

const std::map<MaterialShadingModel, std::string> VERTEX_SHADER = {
    {
        MaterialShadingModel::Unlit,
        R"shader(
#include "ugine_material_unlit.vert.hlsl"
)shader",
    },
    {
        MaterialShadingModel::Lit,
        R"shader(
#include "ugine_material_bp.vert.hlsl"
)shader",
    },
};

const std::map<MaterialShadingModel, std::string> FRAGMENT_SHADER = {
    {
        MaterialShadingModel::Unlit,
        R"shader(
#include "ugine_material_unlit.frag.hlsl"
)shader",
    },
    {
        MaterialShadingModel::Lit,
        R"shader(
#include "ugine_material_bp.frag.hlsl"
)shader",
    },
};

MaterialDescription MaterialBuilder::Parse(const std::filesystem::path& path) {
    // TODO:
    throw std::runtime_error{ "Not implemented" };

    return MaterialDescription{};
}

MaterialBuilder::MaterialBuilder(std::string_view name, const std::filesystem::path& path)
    : name_{ name } {
    Generate(Parse(path));
}

MaterialBuilder::MaterialBuilder(std::string_view name, const MaterialDescription& desc)
    : name_{ name } {
    Generate(desc);
}

void MaterialBuilder::Generate(const MaterialDescription& desc) {
    nlohmann::json json;
    json["name"] = desc.name;
    json["category"] = "material";

    auto& passes{ json["passes"] };

    nlohmann::json pass;
    pass["types"].push_back("Depth");
    pass["types"].push_back("Forward");

    // Features.
    switch (desc.blendMode) {
    case MaterialBlendMode::Opaque: break;
    case MaterialBlendMode::Mask: pass["features"].push_back("Mask"); break;
    case MaterialBlendMode::Translucent:
    case MaterialBlendMode::Additive: pass["features"].push_back("Opacity"); break;
    }

    // Stages.
    nlohmann::json vertexShader;
    vertexShader["stage"] = "VertexShader";
    vertexShader["shader"] = AddVertexShader(desc);

    nlohmann::json fragmentShader;
    fragmentShader["stage"] = "FragmentShader";
    fragmentShader["shader"] = AddFragmentShader(desc);

    auto& stages{ pass["stages"] };
    stages.push_back(vertexShader);
    stages.push_back(fragmentShader);

    // Pipeline.
    auto& pipeline{ pass["pipeline"] };

    // Depth stencil.
    auto& depthStencil{ pipeline["depthStencil"] };
    depthStencil["depthTestEnable"] = true;
    depthStencil["depthWriteEnable"] = true;

    // Blend mode
    auto& blendMode{ pipeline["blending"] };

    switch (desc.blendMode) {
    case MaterialBlendMode::Opaque: blendMode["enable"] = false; break;
    case MaterialBlendMode::Mask: blendMode["enable"] = false; break;
    case MaterialBlendMode::Translucent:
        blendMode["enable"] = true;
        blendMode["srcBlend"] = "srcAlpha";
        blendMode["dstBlend"] = "oneMinusSrcAlpha";
        blendMode["blendOp"] = "add";
        blendMode["srcAlphaBlend"] = "one";
        blendMode["dstAlphaBlend"] = "zero";
        blendMode["alphaBlendOp"] = "add";
        break;
    case MaterialBlendMode::Additive:
        blendMode["enable"] = true;
        blendMode["srcBlend"] = "one";
        blendMode["dstBlend"] = "one";
        blendMode["blendOp"] = "add";
        blendMode["srcAlphaBlend"] = "srcAlpha";
        blendMode["dstAlphaBlend"] = "dstAlpha";
        blendMode["alphaBlendOp"] = "add";
        break;
    }

    // Rasterizer.
    auto& rasterizer{ pipeline["rasterizer"] };
    rasterizer["frontCCW"] = true;

    auto& cullMode{ rasterizer["cullMode"] };
    switch (desc.doubleSided) {
    case true: cullMode = "None"; break;
    case false: cullMode = "Back"; break;
    }

    auto& fillMode{ rasterizer["fillMode"] };
    switch (desc.wireframe) {
    case true: fillMode = "Line"; break;
    case false: fillMode = "Fill"; break;
    }

    passes.push_back(pass);

    // Params.
    auto& params{ json["params"] };
    params["todo"] = 1.0f;

    {
        std::stringstream out;
        out << json;
        descriptor_ = out.str();
    }
}

std::string MaterialBuilder::AddVertexShader(const MaterialDescription& desc) {
    auto source{ VERTEX_SHADER.at(desc.shading) };
    source += desc.source;

    // TODO:
    const auto name{ std::format("{}_vs.hlsl", name_) };

    shaders_.insert(std::make_pair(name, source));
    return name;
}

std::string MaterialBuilder::AddFragmentShader(const MaterialDescription& desc) {
    auto source{ FRAGMENT_SHADER.at(desc.shading) };
    source += desc.source;

    // TODO:
    const auto name{ std::format("{}_fs.hlsl", name_) };

    shaders_.insert(std::make_pair(name, source));
    return name;
}

} // namespace ugine::tools