#include "SerializedMaterial.h"

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/gfx/GraphicsState.h>

#include <gfxapi/Device.h>
#include <gfxapi/Initializers.h>

#include <ugine/engine/core/ResourceManager.h>

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

void serialize(auto& s, SerializedMaterialParam& param) {
    s.value1b(param.type);
    s.container1b(param.value);
}

void serialize(auto& s, MaterialPipeline& pipeline) {
    serialize(s, pipeline.doubleSided);
    serialize(s, pipeline.depthWrite);
    serialize(s, pipeline.depthTest);
    serialize(s, pipeline.wireframe);
    serialize(s, pipeline.blending);
}

void serialize(auto& s, std::pair<u32, ResourceID>& texture) {
    s.value4b(texture.first);
    serialize(s, texture.second);
}

void serialize(auto& s, SerializedMaterial& material) {
    s.text1b(material.name, 128);
    s.value1b(material.isInstance);
    serialize(s, material.shader);
    s.object(material.pipeline);

    if (material.isInstance) {
        serialize(s, material.instanceOf);
    }

    serialize(s, material.shader);
    s.text1b(material.shaderPath, 256);

    s.object(material.params);
}

bool LoadMaterial(Span<const u8> in, SerializedMaterial& out) {
    PROFILE_EVENT();

    auto state{ bitsery::quickDeserialization(InputAdapter{ in.Data(), in.Size() }, out) };
    return state.first == bitsery::ReaderError::NoError;
}

bool SaveMaterial(const SerializedMaterial& in, Vector<u8>& out) {
    PROFILE_EVENT();

    bitsery::quickSerialization(OutputAdapter{ out }, in);
    return true;
}

} // namespace ugine