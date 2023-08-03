#pragma once

#include <gfxapi/Types.h>

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace ugine::gfxapi {

class SpirvCompiler {
public:
    using Defines = std::map<std::string, std::string>;
    using IncludeDirs = std::vector<std::filesystem::path>;

    enum class Optimization {
        None,
        Size,
        Performance,
    };

    struct Options {
        ShaderStage stage;
        std::string source;
        std::string entryPoint;
        std::filesystem::path sourceFile;
        bool preprocess{ true };
        bool warningsAsErrors{};
        bool debugInfo{};
        Optimization optimizationLevel{};
        Defines defines;
        IncludeDirs includeSearchDirs;
    };

    SpirvCompiler() = default;
    virtual ~SpirvCompiler() = default;

    virtual bool Compile(const Options& options) = 0;

    virtual const std::string& Error() const = 0;
    virtual const std::string& ErrorFile() const = 0;
    virtual const std::string& Preprocessed() const = 0;
    virtual int ErrorLine() const = 0;
    virtual int ErrorChar() const = 0;

    virtual const std::vector<u8>& Compiled() const = 0;
};

} // namespace ugine::gfxapi
