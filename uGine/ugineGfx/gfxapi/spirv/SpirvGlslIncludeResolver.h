#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace ugine::gfxapi {

class SpirvGlslIncludeResolver {
public:
    static void IncludePaths(const std::filesystem::path& file, std::vector<std::filesystem::path>& result);

    static const std::vector<std::pair<std::string, std::string>>& BuiltIns();

    bool Process(const std::filesystem::path& file, std::string& src, const std::vector<std::filesystem::path>& includeDirs = {},
        std::set<std::filesystem::path>* includes = nullptr, bool processGlobal = true);

    const std::string& Error() const { return error_; }

private:
    struct Context {
        std::string shaderSource;
        const std::vector<std::filesystem::path>& includeDirectories;
        std::set<std::filesystem::path>* includedFiles;
        bool processGlobal{ true };
    };

    bool ProcessImpl(const std::filesystem::path& file, Context& context, bool isInGlobal);
    bool HandleIncludes(const std::filesystem::path& file, Context& context, bool isInGlobal);
    bool IncludeFile(std::ostream& out, const std::filesystem::path& path, Context& context, bool isGlobal);

    std::string error_;
};

} // namespace ugine::gfxapi
