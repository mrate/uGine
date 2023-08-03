#include <MaterialImporter.h>

#include <ugine/engine/gfx/Material.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>

#include <gfxapi/Json.h>
#include <gfxapi/Types.h>

#include <ugine/File.h>
#include <ugine/Json.h>
#include <ugine/Ugine.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using namespace nlohmann;
using namespace ugine;
using namespace ugine::tools;
using namespace ugine::gfxapi;

namespace ugine {

NLOHMANN_JSON_SERIALIZE_ENUM(MaterialBlending,
    {
        { MaterialBlending::Opaque, "Opaque" },
        { MaterialBlending::Masked, "Masked" },
        { MaterialBlending::Additive, "Additive" },
        { MaterialBlending::Translucent, "Translucent" },
    });

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialPipeline, doubleSided, depthWrite, depthTest, wireframe, blending);

} // namespace ugine

namespace import {

struct Material {
    std::string name;
    std::string layer;
    std::string shader;
    ugine::MaterialPipeline pipeline;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Material, name, layer, shader, pipeline);

} // namespace import

template <typename T> void ParseValue(const nlohmann::json& json, const std::string& name, u8* target, u32 size) {
    const T value{ json.value<T>(name, T{}) };
    memcpy(target, &value, size);
}

namespace ugine::tools {

MaterialImporter::MaterialImporter(const Path& path) noexcept
    : path_{ path } {
}

ugine::SerializedMaterial MaterialImporter::LoadMaterial(const Vector<Path>& includeDirs) {
    const auto baseDir{ path_.ParentPath() };

    // Parse json.
    json j;

    {
        std::ifstream input{ path_.Data() };
        input >> j;
    }

    const auto material{ j.get<import::Material>() };

    ugine::SerializedMaterial output{};
    output.name = material.name;
    output.isInstance = false;
    output.shaderPath = material.shader;
    output.pipeline = material.pipeline;

    //// Default param values.
    //if (j.contains("params")) {
    //    const auto& jsonParams{ j["params"] };

    //    for (const auto& param : jsonParams) {
    //        auto* target{ output.paramValues.data() + param.offset };
    //        const auto size{ param.size };

    //        switch (param.type) {
    //            using enum MaterialParamType;
    //        case Float: ParseValue<float>(jsonParams, param.name, target, size); break;
    //        case Float2: ParseValue<glm::vec2>(jsonParams, param.name, target, size); break;
    //        case Float3: ParseValue<glm::vec3>(jsonParams, param.name, target, size); break;
    //        case Float4: ParseValue<glm::vec4>(jsonParams, param.name, target, size); break;
    //        case Int: ParseValue<int>(jsonParams, param.name, target, size); break;
    //        case Int2: ParseValue<glm::ivec2>(jsonParams, param.name, target, size); break;
    //        case Int3: ParseValue<glm::ivec3>(jsonParams, param.name, target, size); break;
    //        case Int4: ParseValue<glm::ivec4>(jsonParams, param.name, target, size); break;
    //        case Bool: ParseValue<bool>(jsonParams, param.name, target, size); break;
    //        case Matrix4x4: ParseValue<glm::mat4>(jsonParams, param.name, target, size); break;
    //        case Matrix3x3: ParseValue<glm::mat3>(jsonParams, param.name, target, size); break;
    //        case Texture2D:
    //        case Texture3D:
    //        case TextureCube:
    //            // TODO: Import textures?
    //            //output.textures.push_back(std::make_pair(param.binding, jsonParams.value<std::string>(param.name, {}))); break;
    //            output.textures.push_back(std::make_pair(param.binding, ResourceID{}));
    //            break;
    //        default: UGINE_ASSERT(false); break;
    //        }
    //    }
    //}

    return output;
}

} // namespace ugine::tools