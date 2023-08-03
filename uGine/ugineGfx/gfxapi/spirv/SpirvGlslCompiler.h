#pragma once

#include "SpirvCompiler.h"

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace ugine::gfxapi {

class SpirvGlslCompilerImpl;

class SpirvGlslCompiler : public SpirvCompiler {
public:
    explicit SpirvGlslCompiler();
    ~SpirvGlslCompiler();

    SpirvGlslCompiler(SpirvCompiler& other) = delete;
    SpirvGlslCompiler& operator=(SpirvCompiler& other) = delete;

    bool Compile(const Options& options) override;

    const std::string& Error() const override;
    const std::string& Preprocessed() const override;
    int ErrorLine() const;

    const std::vector<u8>& Compiled() const override;

private:
    std::unique_ptr<SpirvGlslCompilerImpl> impl_;
    int errLine_{};
    std::string preprocessed_;
};

} // namespace ugine::gfxapi
