#pragma once

#include "SpirvCompiler.h"

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace ugine::gfxapi {

class SpirvHlslCompilerImpl;

std::string GetTarget(ShaderStage stage, std::string_view version);

class SpirvHlslCompiler : public SpirvCompiler {
public:
    SpirvHlslCompiler();
    ~SpirvHlslCompiler();

    SpirvHlslCompiler(SpirvHlslCompiler& other) = delete;
    SpirvHlslCompiler& operator=(SpirvHlslCompiler& other) = delete;

    bool Compile(const Options& options) override;

    const std::string& Error() const override;
    const std::string& Preprocessed() const override;
    const std::string& ErrorFile() const override;
    int ErrorLine() const override;
    int ErrorChar() const override;

    const std::vector<u8>& Compiled() const override;

private:
    std::unique_ptr<SpirvHlslCompilerImpl> impl_;
};

} // namespace ugine::gfxapi
