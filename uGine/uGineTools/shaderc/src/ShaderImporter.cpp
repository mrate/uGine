#include <ShaderImporter.h>

#include "ShaderDefinition.h"

#include <gfxapi/spirv/SpirvHlslCompiler.h>
#include <gfxapi/spirv/SpirvParser.h>
#include <gfxapi/vulkan/Vulkan.h>

#include <ugine/engine/gfx/Material.h>

#include <ugine/engine/gfx/asset/SerializedShader.h>

#include <ugine/File.h>
#include <ugine/Hash.h>
#include <ugine/Permutations.h>
#include <ugine/StringUtils.h>

#include <filesystem>
#include <format>
#include <iostream>
#include <optional>

using namespace ugine;
using namespace ugine::gfxapi;

using VertexAttributes = std::vector<SerializedVertexAttribute>;

bool ParseShaderStageParams(const std::vector<u8>& shader, ShaderStage stage, SerializedShaderStage& params, VertexAttributes& vertexAttributes) {
    SpirvParser parser{};
    parser.Parse(shader.data(), shader.size(), ugine::gfxapi::ToVulkan(stage));
    const auto& parsedData{ parser.ParsedData() };

    if (stage == ShaderStage::VertexShader) {
        for (const auto& input : parsedData.inputs) {
            vertexAttributes.emplace_back(input.location, input.name);
        }
    }

    for (const auto& ds : parsedData.descriptorSets) {
        auto& dsParams{ params.datasets[ds.set] };

        for (const auto& b : ds.bindings) {
            switch (b.type) {
            case vk::DescriptorType::eUniformBuffer:
                dsParams.uniformSize = b.size;

                for (const auto& m : b.members) {
                    UniformValue::Type type{};
                    switch (m.type) {
                    case ShaderParsedData::Member::Type::Float: type = UniformValue::Type::Float; break;
                    case ShaderParsedData::Member::Type::Float2: type = UniformValue::Type::Float2; break;
                    case ShaderParsedData::Member::Type::Float3: type = UniformValue::Type::Float3; break;
                    case ShaderParsedData::Member::Type::Float4: type = UniformValue::Type::Float4; break;
                    case ShaderParsedData::Member::Type::Int: type = UniformValue::Type::Int; break;
                    case ShaderParsedData::Member::Type::Int2: type = UniformValue::Type::Int2; break;
                    case ShaderParsedData::Member::Type::Int3: type = UniformValue::Type::Int3; break;
                    case ShaderParsedData::Member::Type::Int4: type = UniformValue::Type::Int4; break;
                    case ShaderParsedData::Member::Type::Bool: type = UniformValue::Type::Bool; break;
                    case ShaderParsedData::Member::Type::Matrix3x3: type = UniformValue::Type::Matrix3x3; break;
                    case ShaderParsedData::Member::Type::Matrix4x4: type = UniformValue::Type::Matrix4x4; break;
                    default: continue;
                    }

                    dsParams.params.push_back({
                        .binding = b.binding,
                        .name = m.name,
                        .offset = m.offset,
                        .size = m.size,
                        .type = type,
                    });
                }
                break;
            case vk::DescriptorType::eCombinedImageSampler:
            case vk::DescriptorType::eSampledImage:
                switch (b.dim) {
                case ShaderParsedData::Binding::TexDim::Tex2D: dsParams.params.push_back({ b.binding, b.name, 0, 0, UniformValue::Type::Texture2D }); break;
                case ShaderParsedData::Binding::TexDim::Tex3D: dsParams.params.push_back({ b.binding, b.name, 0, 0, UniformValue::Type::Texture3D }); break;
                case ShaderParsedData::Binding::TexDim::TexCube: dsParams.params.push_back({ b.binding, b.name, 0, 0, UniformValue::Type::TextureCube }); break;
                }
                break;
            case vk::DescriptorType::eSampler: dsParams.params.push_back({ b.binding, b.name, 0, 0, UniformValue::Type::Sampler }); break;
            case vk::DescriptorType::eStorageBuffer: dsParams.params.push_back({ b.binding, b.name, 0, 0, UniformValue::Type::StorageBuffer }); break;
            }
        }
    }

    return true;
}

bool CompileShaderStage(
    const SpirvCompiler::Options& compileOptions, const ShaderImporter::Options& importOptions, std::vector<u8>& output, ShaderImporter::ShaderError& error) {
    SpirvHlslCompiler compiler{};
    const auto result{ compiler.Compile(compileOptions) };

    if (!result) {
        error.defines = compileOptions.defines;
        error.errorLine = compiler.ErrorLine();
        error.message = compiler.Error();
        error.filename = compiler.ErrorFile();
        error.errorChar = compiler.ErrorChar();
        error.shaderProcessedSource = compiler.Preprocessed();

        if (importOptions.verbose) {
            std::cerr << std::format("Compilation error: {} on line {}\n", compiler.Error(), compiler.ErrorLine());
            std::istringstream iss{ compileOptions.source };
            int lineNo{ 1 };
            for (std::string line; std::getline(iss, line); ++lineNo) {
                if (lineNo >= compiler.ErrorLine() - 5 && lineNo <= compiler.ErrorLine() + 5) {
                    std::cerr << std::format("{}[{:04}] {}", compiler.ErrorLine() == lineNo ? ">" : " ", lineNo, line);
                }
            }
        }
        return false;
    }

    output = compiler.Compiled();

    return true;
}

bool ParseShaderStageParams(const SpirvCompiler::Options& compilerOptions, const ShaderImporter::Options& importOptions, SerializedShaderStage& variantStage,
    VertexAttributes& vertexAttributes, ShaderImporter::ShaderError& error) {
    auto debugCompilerOptions{ compilerOptions };
    debugCompilerOptions.debugInfo = true;

    std::vector<u8> withDebug;
    if (!CompileShaderStage(debugCompilerOptions, importOptions, withDebug, error)) {
        return false;
    }

    return ParseShaderStageParams(withDebug, compilerOptions.stage, variantStage, vertexAttributes);
}

bool CompileVariantStage(const ShaderImporter::Options& options, gfxapi::ShaderStage shaderStage, const std::filesystem::path& inputFile,
    std::string_view entryPoint, SerializedShaderVariant& variant, ShaderImporter::ShaderError& error) {

    Path input{ inputFile.string() };
    if (input.IsRelative()) {
        input = Path{ options.inputFile.parent_path().string() } / inputFile.string();
    }

    SpirvCompiler::Options compilerOptions{};
    compilerOptions.debugInfo = true; // TODO:
    compilerOptions.entryPoint = entryPoint;
    compilerOptions.sourceFile = input.Data();
    compilerOptions.includeSearchDirs = options.includeDirs;
    compilerOptions.optimizationLevel = options.optimizeForPerf ? SpirvCompiler::Optimization::Performance : SpirvCompiler::Optimization::None;
    compilerOptions.preprocess = false;
    compilerOptions.stage = shaderStage;

    compilerOptions.includeSearchDirs.push_back(input.ParentPath().Data());

    compilerOptions.source = ugine::ReadFile(input);
    if (compilerOptions.source.empty()) {
        error.filename = input.Data();
        error.message = "File not found";

        if (options.verbose) {
            std::cerr << std::format("Failed to read input shader '{}' / '{}'.\n", input.String(), inputFile.string());
        }
        return false;
    }

    for (const auto& v : variant.defines) {
        compilerOptions.defines[v] = "";
    }

    auto& stage{ variant.stages[shaderStage] };

    if (!ParseShaderStageParams(compilerOptions, options, stage, variant.vertexAttributes, error)) {
        return false;
    }

    std::vector<u8> data;
    if (!CompileShaderStage(compilerOptions, options, data, error)) {
        return false;
    }

    stage.compiled = data;
    stage.entry = entryPoint.data();

    return true;
}

bool CompileVariant(
    const ShaderImporter::Options& options, const ShaderFileDefinition& definition, SerializedShaderVariant& variant, ShaderImporter::ShaderError& error) {
    for (const auto& stage : definition.stages) {
        if (!CompileVariantStage(options, stage.stage, stage.shader, stage.entry, variant, error)) {
            if (options.verbose) {
                std::cerr << "Failed to compile shader stage\n";
            }
            return false;
        }
    }

    return true;
}

void ParseShaderDefaultValues(const ShaderImporter::Options& options, const ShaderFileDefinition& definition, SerializedShader& shader) {
    // TODO: Matrices.

    for (const auto& [key, val] : definition.values.items()) {
        SerializedShaderParamValue value{};

        if (val.is_number_float()) {
            const auto fl{ val.get<float>() };
            memcpy(value.value.data(), &fl, sizeof(float));
            value.type = UniformValue::Type::Float;
        } else if (val.is_number_integer()) {
            const auto fl{ val.get<i32>() };
            memcpy(value.value.data(), &fl, sizeof(float));
            value.type = UniformValue::Type::Int;
        } else if (val.is_array() && !val.empty()) {
            if (val[0].is_number_float()) {
                UGINE_ASSERT(val.size() <= 4);

                const auto len{ std::min<int>(4, int(val.size())) };

                glm::vec4 vec;
                for (int i{}; i < len; ++i) {
                    vec[i] = val[i].get<float>();
                }

                memcpy(value.value.data(), &vec, sizeof(float) * len);

                switch (len) {
                case 1: value.type = UniformValue::Type::Float; break;
                case 2: value.type = UniformValue::Type::Float2; break;
                case 3: value.type = UniformValue::Type::Float3; break;
                default: value.type = UniformValue::Type::Float4; break;
                }
            } else if (val[0].is_number_integer()) {
                UGINE_ASSERT(val.size() <= 4);

                const auto len{ std::min<int>(4, int(val.size())) };

                glm::ivec4 vec;
                for (int i{}; i < len; ++i) {
                    vec[i] = val[i].get<i32>();
                }

                memcpy(value.value.data(), &vec, sizeof(i32) * len);

                switch (len) {
                case 1: value.type = UniformValue::Type::Int; break;
                case 2: value.type = UniformValue::Type::Int2; break;
                case 3: value.type = UniformValue::Type::Int3; break;
                default: value.type = UniformValue::Type::Int4; break;
                }
            } else {
                if (options.verbose) {
                    std::cerr << std::format("Failed to parse default value '{}'\n", key);
                }
                continue;
            }
        } else {
            if (options.verbose) {
                std::cerr << std::format("Failed to parse default value '{}'\n", key);
            }
            continue;
        }

        shader.params[key] = value;
    }
}

bool Build(const ShaderImporter::Options& options, const ShaderFileDefinition& definition, SerializedShader& shader,
    std::vector<ShaderImporter::ShaderError>& shaderErrors) {
    shader.name = definition.name;
    shader.category = definition.category;
    shader.defines = definition.defines;

    Permutations<std::string> permutations{ Span<const std::string>{ definition.permutations.data(), definition.permutations.size() } };

    while (!permutations.End()) {
        const auto definePermutation{ permutations.Next() };

        SerializedShaderVariant variant{};

        for (const auto& define : definition.defines) {
            variant.defines.push_back(define);
        }

        for (const auto& define : definePermutation) {
            variant.defines.push_back(define);
        }

        ShaderImporter::ShaderError error;
        if (!CompileVariant(options, definition, variant, error)) {
            shaderErrors.push_back(error);

            return false;
        }

        shader.variants.push_back(variant);

        if (options.saveSteps) {
            const auto dir{ options.outputFile.parent_path() };

            for (const auto& stage : variant.stages) {
                auto fileName{ options.outputFile.filename() };
                fileName.replace_extension(std::to_string(shader.variants.size()) + "." + GetTarget(stage.first, "6_0"));

                const auto outFile{ dir / fileName };
                WriteFileBinary(Path{ outFile.string() }, Span<const u8>{ stage.second.compiled.data(), stage.second.compiled.size() });
            }
        }
    }

    ParseShaderDefaultValues(options, definition, shader);

    return true;
}

bool SaveShader(const SerializedShader& shader, const ShaderImporter::Options& options) {
    Vector<u8> out;
    SaveShader(shader, out);
    WriteFileBinary(Path{ options.outputFile.string() }, out);
    return true;
}

bool ShaderImporter::Import(const ShaderImporter::Options& options) {
    ShaderFileDefinition definition{};
    if (!Parse(options.inputFile, definition)) {
        error_ = "Failed to parse shader definition file.";
        return false;
    }

    SerializedShader shader{};
    if (!Build(options, definition, shader, shaderErrors_)) {
        error_ = "Shader build failed.";
        return false;
    }

    const auto outputDir{ options.outputFile.parent_path() };
    if (!std::filesystem::exists(outputDir)) {
        std::error_code ec{};
        std::filesystem::create_directories(outputDir, ec);

        // TODO: Error handling.
    }

    if (!SaveShader(shader, options)) {
        error_ = "Failed to save shader";
        return false;
    }

    return true;
}