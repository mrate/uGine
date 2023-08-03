#pragma once

#include <ugine/engine/core/ResourceID.h>

namespace ugine {

struct UniformValue {
    static constexpr auto MAX_SIZE{ sizeof(float) * 4 * 4 };

    enum class Type : u8 {
        Float = 0,
        Float2,
        Float3,
        Float4,
        Int,
        Int2,
        Int3,
        Int4,
        Bool,
        Matrix4x4,
        Matrix3x3,
        TextureID, // Bindless
        Texture2D,
        Texture3D,
        TextureCube,
        Sampler,
        StorageBuffer,
    };

    int Size() const {
        switch (type) {
        case Type::Float: return sizeof(float);
        case Type::Float2: return sizeof(glm::vec2);
        case Type::Float3: return sizeof(glm::vec3);
        case Type::Float4: return sizeof(glm::vec4);
        case Type::Int: return sizeof(i32);
        case Type::Int2: return sizeof(glm::ivec2);
        case Type::Int3: return sizeof(glm::ivec3);
        case Type::Int4: return sizeof(glm::ivec4);
        case Type::Bool: return sizeof(u32);
        case Type::Matrix4x4: return sizeof(glm::mat4);
        case Type::Matrix3x3: return sizeof(glm::mat3);
        case Type::TextureID: return sizeof(ResourceID);
        default: UGINE_ASSERT(false); return 0;
        }
    }

    template <typename T> const T& Get() const { return reinterpret_cast<const T&>(value); }

    StringID name{};
    Type type{ Type::Float };
    std::array<u8, MAX_SIZE> value{};
};

} // namespace ugine