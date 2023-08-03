#pragma once

#include <ugine/Ugine.h>

#include <vulkan/vulkan.hpp>

#include <filesystem>

#include <stdint.h>

namespace ugine::gfxapi {

struct ShaderParsedData {
    struct Member {
        enum class Type {
            Float,
            Float2,
            Float3,
            Float4,
            Matrix3x3,
            Matrix4x4,
            Int,
            Int2,
            Int3,
            Int4,
            Bool,
            Array,
        };

        Type type;
        u32 offset;
        u32 size;
        std::string name;
    };

    struct Binding {
        enum class TexDim {
            Tex2D,
            Tex3D,
            TexCube,
        };

        u32 binding{};
        vk::DescriptorType type{};
        std::string name;
        std::string typeName;
        std::vector<Member> members;
        u32 size{};
        TexDim dim{};
        u32 count{};
        bool isArray{};
        vk::ShaderStageFlags stages{};
    };

    struct DescriptorSet {
        u32 set;
        std::vector<Binding> bindings;
    };

    struct PushConstant {
        vk::ShaderStageFlags stage{};
        u32 offset{};
        u32 size{};
    };

    struct InterfaceVariable {
        std::string name;
        u32 location{};
        vk::ShaderStageFlags stage{};
    };

    std::vector<DescriptorSet> descriptorSets;
    std::vector<PushConstant> pushConstants;
    std::vector<InterfaceVariable> inputs;
};

class SpirvParser {
public:
    SpirvParser() = default;

    void Parse(const void* data, size_t size, vk::ShaderStageFlags stage);

    const ShaderParsedData& ParsedData() const { return data_; }

private:
    void AddDescriptorSet(const ShaderParsedData::DescriptorSet& ds);

    ShaderParsedData data_;
};

} // namespace ugine::gfxapi
