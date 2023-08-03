#pragma once

#include <ugine/engine/gfx/Material.h>

#include <filesystem>
#include <map>
#include <string>

namespace ugine::tools {

enum class MaterialDomain : u8 {
    Surface = 0,
    PostProcess,
};

enum class MaterialShadingModel : u8 {
    Unlit = 0,
    Lit,
};

enum class MaterialBlendMode : u8 {
    Opaque = 0,
    Mask,
    Translucent,
    Additive,
};

struct MaterialDescription {
    std::string name;
    std::string source;
    MaterialDomain domain{ MaterialDomain::Surface };
    MaterialShadingModel shading{ MaterialShadingModel::Unlit };
    MaterialBlendMode blendMode{ MaterialBlendMode::Opaque };
    bool doubleSided{};
    bool wireframe{};
};

class MaterialBuilder {
public:
    static MaterialDescription Parse(const std::filesystem::path& path);

    MaterialBuilder(std::string_view name, const std::filesystem::path& path);
    MaterialBuilder(std::string_view name, const MaterialDescription& desc);

    const std::string& Descriptor() const { return descriptor_; }
    const std::map<std::string, std::string> Shaders() const { return shaders_; }

private:
    void Generate(const MaterialDescription& desc);

    std::string AddVertexShader(const MaterialDescription& desc);
    std::string AddFragmentShader(const MaterialDescription& desc);

    std::string name_;
    std::string descriptor_;
    std::map<std::string, std::string> shaders_;
};

} // namespace ugine::tools