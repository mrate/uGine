#pragma once

#include <ugine/engine/gfx/Material.h>

#include <ugine/Span.h>
#include <ugine/Vector.h>

#include <gfxapi/Types.h>

#include <filesystem>
#include <map>
#include <string>

namespace ugine {

struct SerializedMaterialParam {
    UniformValue::Type type{};
    std::array<u8, UniformValue::MAX_SIZE> value;
};

struct SerializedMaterial {
    std::string name{}; // TODO: remove.
    u8 isInstance{};

    ResourceID instanceOf;

    ResourceID shader;
    std::string shaderPath;

    MaterialPipeline pipeline;

    std::map<u64, SerializedMaterialParam> params;
};

bool LoadMaterial(Span<const u8> in, SerializedMaterial& out);
bool SaveMaterial(const SerializedMaterial& in, Vector<u8>& out);

} // namespace ugine