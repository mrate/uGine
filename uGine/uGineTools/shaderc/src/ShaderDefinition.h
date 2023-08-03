#pragma once

#include <gfxapi/Json.h>
#include <gfxapi/Types.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

//{
//  "name": "Bone",
//  "category": "material",
//  "defines": [ "MATERIAL_OPACITY" ],
//  "permuations": [ "DEPTH", "INSTANCED" ],
//  "stages": [
//    {
//      "stage": "VertexShader",
//      "shader": "bone.vert.hlsl"
//    },
//    {
//      "stage": "FragmentShader",
//      "shader": "bone.frag.hlsl"
//    }
//  ],
//  "default": {
//      "name": ...
//  }
//}

struct ShaderStageDefinition {
    ugine::gfxapi::ShaderStage stage;
    std::string shader;
    std::string entry{ "main" };
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShaderStageDefinition, stage, shader);

struct ShaderFileDefinition {
    std::string name;
    std::string category;
    std::vector<std::string> defines;
    std::vector<std::string> permutations;
    std::vector<ShaderStageDefinition> stages;
    nlohmann::json values;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShaderFileDefinition, name, category, defines, permutations, stages, values);

bool Parse(const std::filesystem::path& file, ShaderFileDefinition& output);
