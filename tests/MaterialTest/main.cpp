#include <MaterialBuilder.h>
#include <MaterialImporter.h>

#include <format>
#include <fstream>
#include <iostream>

using namespace ugine::tools;

const MaterialDescription unlitMaterial{
    .name = "My Unlit Material",
    .source = R"source(
layout(set = DATASET_MATERIAL, binding = 0) uniform Params {
    float value;
}
material;

layout(set = DATASET_MATERIAL, binding = 1) uniform sampler2D emissiveTexture;

vec3 material_vertex_offset() { return vec3(0); }

void material_unlit_fragment(inout MaterialUnlitFragment fragment, vec2 uv) {
    fragment.emissive = texture(emissiveTexture, uv).rgb;
#ifdef MATERIAL_MASK
    fragment.mask = 1;
#endif
}
)source",
    .domain = MaterialDomain::Surface,
    .shading = MaterialShadingModel::Unlit,
    .blendMode = MaterialBlendMode::Mask,
    .doubleSided = true,
    .wireframe = false,
};

const MaterialDescription shadedMaterial{
    .name = "My Shaded Material",
    .source = R"source(
layout(set = DATASET_MATERIAL, binding = 0) uniform Params {
    float value;
}
material;

layout(set = DATASET_MATERIAL, binding = 1) uniform sampler2D albedoTexture;
layout(set = DATASET_MATERIAL, binding = 2) uniform sampler2D normalTexture;
layout(set = DATASET_MATERIAL, binding = 3) uniform sampler2D emissiveTexture;

vec3 material_vertex_offset() { return vec3(0); }

void material_bp_fragment(inout MaterialBpFragment fragment, vec2 uv) {
    fragment.albedo = texture(albedoTexture, uv).rgb;
    fragment.emissive = texture(emissiveTexture, uv).rgb;
    fragment.normal = normalize(texture(normalTexture, uv).rgb - 0.5) * 2.0;
#ifdef MATERIAL_MASK
    fragment.mask = 1;
#endif
}
)source",
    .domain = MaterialDomain::Surface,
    .shading = MaterialShadingModel::Lit,
    .blendMode = MaterialBlendMode::Opaque,
    .doubleSided = false,
    .wireframe = false,
};

void TestMaterial(std::string_view file, const MaterialDescription& desc) {
    std::cout << std::format("Testing material '{}'\n", file);

    const ugine::Path materialFile{ std::string{ file } + ".mat" };
    const ugine::Path outputFile{ std::string{ file } + ".umat" };

    std::cout << "Parsing material...";

    MaterialBuilder builder{ file, desc };
    const auto& json{ builder.Descriptor() };

    std::cout << "OK\n";

    std::cout << "Saving material...";
    {
        std::ofstream out{ materialFile.Data() };
        out << json;
    }

    for (const auto& [shader, source] : builder.Shaders()) {
        std::ofstream out{ shader };
        out << source;
    }

    std::cout << "OK\n";

    std::cout << "Building material...";
    const ugine::Vector<ugine::Path> includes = { ugine::FileSystem::CurrentPath() / ".." / "ugine" / "ugine" / "shaders" };
    MaterialImporter importer{ materialFile };
    auto serialized{ importer.LoadMaterial(includes) };

    // TODO: [resources]
    //ugine::SaveSerialized(outputFile, serialized);
    std::cout << "OK\n";
}

int main(int argc, char* argv[]) {
    try {
        TestMaterial("unlitMaterial", unlitMaterial);
        TestMaterial("shadedMaterial", shadedMaterial);
    } catch (const std::exception& ex) {
        std::cerr << std::format("Error: {}\n", ex.what());
        return -1;
    }

    return 0;
}