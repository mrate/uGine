#include "SerializedShader.h"

#include <ugine/Error.h>
#include <ugine/File.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/Serialization.h>

#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/brief_syntax.h>
#include <bitsery/brief_syntax/map.h>
#include <bitsery/brief_syntax/vector.h>
#include <bitsery/ext/std_map.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

namespace ugine {

void serialize(auto& s, std::pair<ugine::gfxapi::ShaderStage, std::vector<u8>>& data) {
    static constexpr auto MAX_SHADER_SIZE{ 1024 * 1024 * 1024 };

    UGINE_ASSERT(data.second.size() <= MAX_SHADER_SIZE);

    s.value1b(data.first);
    s.container1b(data.second, MAX_SHADER_SIZE);
}

void serialize(auto& s, SerializedShaderStage& stage) {
    s.object(stage.source);
    s.object(stage.compiled);
    s.text1b(stage.entry, 64);
    s.object(stage.datasets);
}

void serialize(auto& s, SerializedVertexAttribute& vs) {
    s.object(vs.location);
    s.text1b(vs.name, 256);
}

void serialize(auto& s, SerializedShaderParamDescriptor& param) {
    serialize(s, param.binding);
    s.text1b(param.name, 256);
    serialize(s, param.offset);
    serialize(s, param.size);
    s.value1b(param.type);
}

void serialize(auto& s, SerializedShaderParamValue& value) {
    s.value1b(value.type);
    s.container1b(value.value);
}

void serialize(auto& s, SerializedDatasetParams& param) {
    UGINE_ASSERT(param.params.size() <= 16);
    s.container(param.params, 16);
    serialize(s, param.uniformSize);
}

void serialize(auto& s, SerializedShaderVariant& variant) {
    s.object(variant.stages);
    s(variant.defines);

    UGINE_ASSERT(variant.vertexAttributes.size() <= 16);
    s.container(variant.vertexAttributes, 16);
}

void serialize(auto& s, SerializedShader& shader) {
    s.text1b(shader.name, 128);
    s.text1b(shader.category, 64);

    s(shader.variants);
    s.object(shader.defines);
    s(shader.params);
}

bool LoadShader(Span<const u8> in, SerializedShader& out) {
    PROFILE_EVENT();

    auto state{ bitsery::quickDeserialization(InputAdapter{ in.Data(), in.Size() }, out) };
    return state.first == bitsery::ReaderError::NoError;
}

bool SaveShader(const SerializedShader& in, Vector<u8>& out) {
    PROFILE_EVENT();

    bitsery::quickSerialization(OutputAdapter{ out }, in);
    return true;
}

} // namespace ugine