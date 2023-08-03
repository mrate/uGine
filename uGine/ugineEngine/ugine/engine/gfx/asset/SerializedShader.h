#pragma once

#include <ugine/Ugine.h>

#include <ugine/engine/gfx/Shader.h>

#include <ugine/Span.h>
#include <ugine/Vector.h>

#include <gfxapi/Types.h>

#include <map>
#include <string>
#include <vector>

namespace ugine {

struct SerializedVertexAttribute {
    u32 location;
    std::string name;
};

struct SerializedShaderParamDescriptor {
    u32 binding{};
    std::string name;
    u32 offset{};
    u32 size{};
    UniformValue::Type type{};
};

struct SerializedDatasetParams {
    std::vector<SerializedShaderParamDescriptor> params;
    u32 uniformSize;
};

struct SerializedShaderStage {
    std::vector<char> source;
    std::vector<u8> compiled;
    std::string entry;
    std::map<u32, SerializedDatasetParams> datasets;
};

struct SerializedShaderVariant {
    std::map<gfxapi::ShaderStage, SerializedShaderStage> stages;
    std::vector<std::string> defines;
    std::vector<SerializedVertexAttribute> vertexAttributes;
};

struct SerializedShaderParamValue {
    UniformValue::Type type;
    std::array<u8, UniformValue::MAX_SIZE> value;
};

struct SerializedShader {
    std::string name; // TODO: Remove.
    std::string category;
    std::vector<SerializedShaderVariant> variants;
    std::vector<std::string> defines;
    std::map<std::string, SerializedShaderParamValue> params;
};

bool LoadShader(Span<const u8> in, SerializedShader& out);
bool SaveShader(const SerializedShader& in, Vector<u8>& out);

} // namespace ugine