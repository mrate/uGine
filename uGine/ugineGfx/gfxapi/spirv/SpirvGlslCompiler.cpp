#include "SpirvGlslCompiler.h"
#include "SpirvGlslIncludeResolver.h"

#include <gfxapi/Error.h>
#include <gfxapi/vulkan/Vulkan.h>

#include <ugine/Log.h>

#include <shaderc/shaderc.hpp>

#include <fstream>

namespace {

shaderc_shader_kind FromStage(vk::ShaderStageFlags stage) {
    if (stage == vk::ShaderStageFlagBits::eVertex) {
        return shaderc_glsl_vertex_shader;
    } else if (stage == vk::ShaderStageFlagBits::eGeometry) {
        return shaderc_glsl_geometry_shader;
    } else if (stage == vk::ShaderStageFlagBits::eTessellationControl) {
        return shaderc_glsl_tess_control_shader;
    } else if (stage == vk::ShaderStageFlagBits::eTessellationEvaluation) {
        return shaderc_glsl_tess_evaluation_shader;
    } else if (stage == vk::ShaderStageFlagBits::eFragment) {
        return shaderc_glsl_fragment_shader;
    } else if (stage == vk::ShaderStageFlagBits::eCompute) {
        return shaderc_glsl_compute_shader;
    } else {
        UGINE_THROW(ugine::gfxapi::GfxError, "Invalid shader stage passed to compiler.");
    }
}

} // namespace

namespace ugine::gfxapi {

class SpirvGlslCompilerImpl {
public:
    SpirvGlslCompilerImpl() {}
    ~SpirvGlslCompilerImpl() {}

    void GenerateDebugInfo() { options_.SetGenerateDebugInfo(); }

    void SetDefines(const SpirvCompiler::Defines& defines) {
        for (auto [name, val] : defines) {
            options_.AddMacroDefinition(name, val);
        }
    }

    void SetOptimizationLevel(SpirvGlslCompiler::Optimization level) {
        switch (level) {
        case SpirvCompiler::Optimization::None: options_.SetOptimizationLevel(shaderc_optimization_level_zero); break;
        case SpirvCompiler::Optimization::Size: options_.SetOptimizationLevel(shaderc_optimization_level_size); break;
        case SpirvCompiler::Optimization::Performance: options_.SetOptimizationLevel(shaderc_optimization_level_performance); break;
        }
    }

    bool Compile(const SpirvCompiler::Options& options) {
        //UGINE_PROFILE("ShaderCompile");

        if (options.warningsAsErrors) {
            options_.SetWarningsAsErrors();
        }
        if (options.debugInfo) {
            GenerateDebugInfo();
        }
        SetOptimizationLevel(options.optimizationLevel);

        preprocessed_ = options.source;

        const auto kind{ FromStage(ToVulkan(options.stage)) };

        const auto fileName{ options.sourceFile.filename() };
        if (options.preprocess) {
            SpirvGlslIncludeResolver prep{};
            if (!prep.Process(fileName, preprocessed_, options.includeSearchDirs, nullptr)) {
                error_ = prep.Error();
                return false;
            }

            const auto result{ compiler_.PreprocessGlsl(preprocessed_, kind, fileName.string().c_str(), options_) };
            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                error_ = std::format("Failed to preprocess shader {}", fileName.string());
                return false;
            }

            preprocessed_ = std::string{ result.cbegin(), result.cend() };
            //UGINE_DEBUG("PREPROCESSED: {}", preprocessed_);
        }

        const auto result{ compiler_.CompileGlslToSpv(preprocessed_, kind, fileName.string().c_str(), options.entryPoint.c_str(), options_) };

        if (result.GetNumErrors()) {
            error_ = result.GetErrorMessage();

            std::string errFile{};
            ParseError(error_, errFile, errLine_);

            //UGINE_DEBUG("Failed to compile shader {}:", options.fileName.string());
            //UGINE_DEBUG(error_.c_str());
            //std::istringstream in{ preprocessed_ };
            //int lineNo{ 1 };
            //for (std::string line; std::getline(in, line); lineNo++) {
            //    if (lineNo > errLine_ - 3 && lineNo < errLine_ + 3) {
            //        if (errLine_ == lineNo) {
            //            UGINE_ERROR(">{:4}: {}", lineNo, line);
            //        } else {
            //            UGINE_ERROR(" {:4}: {}", lineNo, line);
            //        }
            //    } else if (lineNo >= errLine_ + 3) {
            //        break;
            //    }
            //}
            //UGINE_ERROR("");

            return false;
        }

        if (result.GetNumWarnings()) {
            error_ = result.GetErrorMessage();
            UGINE_WARN("Shader '{}' warnings: {}", fileName.string(), error_);
        }

        std::vector<u32> data32{ result.cbegin(), result.cend() };
        data_.resize(data32.size() * 4);

        for (auto it = data_.begin(); const auto value : data32) {
            *it++ = (value >> 0) & 0xff;
            *it++ = (value >> 8) & 0xff;
            *it++ = (value >> 16) & 0xff;
            *it++ = (value >> 24) & 0xff;
        }

        return true;
    }

    void ParseError(const std::string& error, std::string& file, int& line) const {
        auto pos{ error.find(':') };
        if (pos != std::string::npos) {
            file = error.substr(0, pos);

            line = 0;
            while (++pos < error.size() && error[pos] >= '0' && error[pos] <= '9') {
                line = line * 10 + (error[pos] - '0');
            }
        }
    }

    shaderc::CompileOptions options_{};
    shaderc::Compiler compiler_{};
    std::string error_;
    std::vector<u8> data_;
    int errLine_;
    std::string preprocessed_;
};

SpirvGlslCompiler::SpirvGlslCompiler()
    : impl_{ std::make_unique<SpirvGlslCompilerImpl>() } {
}

SpirvGlslCompiler::~SpirvGlslCompiler() {
}

bool SpirvGlslCompiler::Compile(const Options& options) {
    return impl_->Compile(options);
}

const std::string& SpirvGlslCompiler::Error() const {
    return impl_->error_;
}

const std::vector<u8>& SpirvGlslCompiler::Compiled() const {
    return impl_->data_;
}

const std::string& SpirvGlslCompiler::Preprocessed() const {
    return impl_->preprocessed_;
}

int SpirvGlslCompiler::ErrorLine() const {
    return impl_->errLine_;
}

} // namespace ugine::gfxapi
