#include "SpirvParser.h"

#include <gfxapi/Error.h>

#include <gfxapi/spirv/SpirvCompiler.h>

#include <ugine/Align.h>
#include <ugine/File.h>
#include <ugine/Log.h>

#include <common/output_stream.h>
#include <spirv_reflect.h>

#define CHECK(result)                                                                                                                                          \
    {                                                                                                                                                          \
        if (result != SPV_REFLECT_RESULT_SUCCESS) {                                                                                                            \
            UGINE_THROW(GfxError, "Shader parser error.");                                                                                                     \
        }                                                                                                                                                      \
    }

namespace {

constexpr u32 BITS_TO_BYTES(u32 bits) {
    return bits >> 3;
}

class ScopedModule {
public:
    ScopedModule(SpvReflectShaderModule& reflectModule)
        : reflectModule_(reflectModule) {}

    ~ScopedModule() { spvReflectDestroyShaderModule(&reflectModule_); }

private:
    SpvReflectShaderModule& reflectModule_;
};

void ParseMembers(ugine::gfxapi::ShaderParsedData::Binding& b, const SpvReflectDescriptorBinding& binding) {
    using namespace ugine::gfxapi;

    u32 totalSize{ 0 };
    for (u32 i = 0; i < binding.type_description->member_count; ++i) {
        auto& member{ binding.type_description->members[i] };

        ShaderParsedData::Member newMember{};
        newMember.name = member.struct_member_name;

        if (member.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            // Matrix.
            const auto rows{ member.traits.numeric.matrix.row_count };
            const auto cols{ member.traits.numeric.matrix.column_count };

            if (rows == 4 && cols == 4) {
                newMember.type = ShaderParsedData::Member::Type::Matrix4x4;
            } else if (rows == 3 && cols == 3) {
                newMember.type = ShaderParsedData::Member::Type::Matrix3x3;
            } else {
                UGINE_THROW(GfxError, "Unsupported matrix dimensions.");
            }

            newMember.size = BITS_TO_BYTES(rows * cols * member.traits.numeric.scalar.width);
        } else if (member.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            // Vector.
            if (member.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
                switch (member.traits.numeric.vector.component_count) {
                case 2: newMember.type = ShaderParsedData::Member::Type::Float2; break;
                case 3: newMember.type = ShaderParsedData::Member::Type::Float3; break;
                case 4: newMember.type = ShaderParsedData::Member::Type::Float4; break;
                default: UGINE_THROW(GfxError, "Unsupported vector component count.");
                }
            } else if (member.type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
                switch (member.traits.numeric.vector.component_count) {
                case 2: newMember.type = ShaderParsedData::Member::Type::Int2; break;
                case 3: newMember.type = ShaderParsedData::Member::Type::Int3; break;
                case 4: newMember.type = ShaderParsedData::Member::Type::Int4; break;
                default: UGINE_THROW(GfxError, "Unsupported vector component count.");
                }
            } else {
                UGINE_THROW(GfxError, "Unsupported vector base type.");
            }

            if (member.type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
                //UGINE_WARN("Error parsing shader. Member {} is array of vectors.", member.struct_member_name);
                newMember.type = ShaderParsedData::Member::Type::Array;
            }
            newMember.size = BITS_TO_BYTES(member.traits.numeric.vector.component_count * member.traits.numeric.scalar.width);
        } else if (member.type_flags == SPV_REFLECT_TYPE_FLAG_FLOAT) {
            // Scalar Float.
            newMember.type = ShaderParsedData::Member::Type::Float;
            newMember.size = BITS_TO_BYTES(member.traits.numeric.scalar.width);
        } else if (member.type_flags == SPV_REFLECT_TYPE_FLAG_INT) {
            // Scalar Int.
            newMember.type = ShaderParsedData::Member::Type::Int;
            newMember.size = BITS_TO_BYTES(member.traits.numeric.scalar.width);
        } else if (member.type_flags == SPV_REFLECT_TYPE_FLAG_BOOL) {
            // Scalar Bool.
            newMember.type = ShaderParsedData::Member::Type::Bool;
            newMember.size = BITS_TO_BYTES(member.traits.numeric.scalar.width);
        } else if (member.type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
            newMember.type = ShaderParsedData::Member::Type::Array;
            newMember.size = 0; // TODO:
            //UGINE_THROW(GfxError, "Unsupported member type.");
            continue; // TODO:
        } else if (member.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) {
            continue;
        } else {
            UGINE_THROW(GfxError, "Unsupported member type.");
        }

        totalSize = ugine::AlignBy(totalSize, newMember.size, 16) + newMember.size;
        newMember.offset = totalSize - newMember.size;

        b.members.push_back(newMember);
    }

    b.size = totalSize;
}

} // namespace

namespace ugine::gfxapi {

//SpirvParser::SpirvParser(vk::ShaderStageFlags stage, const std::filesystem::path& file, bool binary) {
//    if (binary) {
//        auto content{ utils::ReadFileBinary(file) };
//        Parse(content.data(), content.size());
//    } else {
//        std::string text{ utils::ReadFile(file) };
//        std::string fileName{ file.string() };
//
//        SpirvCompiler compiler;
//        if (!compiler.Compile(stage, text, "main", fileName)) {
//            UGINE_ERROR("Error pasing shader. Compile error: {}", compiler.Error());
//            UGINE_THROW(GfxError, "Failed to parse shader.");
//        }
//
//        Parse(compiler.Data().data(), compiler.Data().size());
//    }
//}

void SpirvParser::Parse(const void* data, size_t size, vk::ShaderStageFlags stage) {
    SpvReflectResult result{};
    SpvReflectShaderModule reflectModule{};

    result = spvReflectCreateShaderModule(size, data, &reflectModule);
    CHECK(result);

    ScopedModule scopedHolder(reflectModule);

    u32 count = 0;
    result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, nullptr);
    CHECK(result);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, sets.data());
    CHECK(result);

    for (size_t index = 0; index < sets.size(); ++index) {
        auto p_set = sets[index];

        // descriptor sets can also be retrieved directly from the module, by set index
        auto p_set2 = spvReflectGetDescriptorSet(&reflectModule, p_set->set, &result);
        CHECK(result);

        ShaderParsedData::DescriptorSet ds{};
        ds.set = p_set->set;

        for (u32 i = 0; i < p_set->binding_count; ++i) {
            const SpvReflectDescriptorBinding& binding = *p_set->bindings[i];

            ShaderParsedData::Binding parsedBinding{};
            parsedBinding.binding = binding.binding;
            parsedBinding.name = binding.name ? binding.name : "";
            if (binding.type_description->type_name) {
                parsedBinding.typeName = binding.type_description->type_name;
            }
            parsedBinding.size = 0;
            parsedBinding.count = binding.count;

            switch (binding.descriptor_type) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                parsedBinding.type = vk::DescriptorType::eUniformBuffer;
                ParseMembers(parsedBinding, binding);
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: parsedBinding.type = vk::DescriptorType::eStorageBuffer; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                parsedBinding.type = binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE ? vk::DescriptorType::eSampledImage
                                                                                                          : vk::DescriptorType::eCombinedImageSampler;
                switch (binding.image.dim) {
                case SpvDim::SpvDim2D: parsedBinding.dim = ShaderParsedData::Binding::TexDim::Tex2D; break;
                case SpvDim::SpvDim3D: parsedBinding.dim = ShaderParsedData::Binding::TexDim::Tex3D; break;
                case SpvDim::SpvDimCube: parsedBinding.dim = ShaderParsedData::Binding::TexDim::TexCube; break;
                default: UGINE_THROW(GfxError, "Unsupported image sampler dimensions.");
                }
                parsedBinding.isArray = binding.image.arrayed;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: parsedBinding.type = vk::DescriptorType::eStorageImage; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: parsedBinding.type = vk::DescriptorType::eSampler; break;
            default: UGINE_THROW(GfxError, "Unsupported storage type.");
            }

            parsedBinding.stages = stage;
            ds.bindings.push_back(parsedBinding);
        }

        AddDescriptorSet(ds);
    }

    count = 0;
    result = spvReflectEnumeratePushConstantBlocks(&reflectModule, &count, nullptr);
    CHECK(result);

    std::vector<SpvReflectBlockVariable*> blocks(count);
    result = spvReflectEnumeratePushConstantBlocks(&reflectModule, &count, blocks.data());
    CHECK(result);

    for (size_t index = 0; index < blocks.size(); ++index) {
        const auto& block{ blocks[index] };
        data_.pushConstants.push_back({ stage, block->offset, block->size });
    }

    count = 0;
    result = spvReflectEnumerateInputVariables(&reflectModule, &count, nullptr);
    CHECK(result);

    std::vector<SpvReflectInterfaceVariable*> inputs(count);
    result = spvReflectEnumerateInputVariables(&reflectModule, &count, inputs.data());
    CHECK(result);

    for (const auto& input : inputs) {
        if (input->built_in < 0) {
            data_.inputs.push_back({ .name = input->name, .location = input->location, .stage = stage });
        }
    }
}

void SpirvParser::AddDescriptorSet(const ShaderParsedData::DescriptorSet& ds) {
    auto it{ std::find_if(
        data_.descriptorSets.begin(), data_.descriptorSets.end(), [set = ds.set](const auto& descriptorSet) { return descriptorSet.set == set; }) };

    if (it == data_.descriptorSets.end()) {
        data_.descriptorSets.push_back(ds);
    } else {
        // Merge descriptors.
        auto& descriptorSet{ *it };

        for (const auto& binding : ds.bindings) {
            auto it{ std::find_if(
                descriptorSet.bindings.begin(), descriptorSet.bindings.end(), [b = binding.binding](const auto& bind) { return bind.binding == b; }) };

            if (it == descriptorSet.bindings.end()) {
                descriptorSet.bindings.push_back(binding);
            } else {
                // TODO: Check bindings are same.
                it->stages |= binding.stages;
            }
        }
    }
}

} // namespace ugine::gfxapi
