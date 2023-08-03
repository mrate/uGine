#include "SpirvGlslIncludeResolver.h"

#include <ugine/File.h>
#include <ugine/Ugine.h>

#include <regex>
#include <sstream>

namespace ugine::gfxapi {

void SpirvGlslIncludeResolver::IncludePaths(const std::filesystem::path& file, std::vector<std::filesystem::path>& result) {
    const auto directory{ file.parent_path() };
    const auto src{ ReadFile(file.string().c_str()) };

    std::regex reInclude{ "^#include \"(.*)\"$" };

    for (std::sregex_iterator i{ src.begin(), src.end(), reInclude }, end{}; i != end; ++i) {
        std::smatch matches{ *i };

        const auto file{ directory / matches.str(1) };
        result.push_back(file);

        IncludePaths(file, result);
    }
}

const std::vector<std::pair<std::string, std::string>>& SpirvGlslIncludeResolver::BuiltIns() {
    static std::vector<std::pair<std::string, std::string>> vars{};
    return vars;
}

bool SpirvGlslIncludeResolver::Process(const std::filesystem::path& file, std::string& src, const std::vector<std::filesystem::path>& includeDirs,
    std::set<std::filesystem::path>* includes, bool processGlobal) {
    Context context{
        .shaderSource = src,
        .includeDirectories = includeDirs,
        .includedFiles = includes,
        .processGlobal = processGlobal,
    };

    if (!ProcessImpl(file, context, false)) {
        return false;
    }

    src = context.shaderSource;
    return true;
}

bool SpirvGlslIncludeResolver::ProcessImpl(const std::filesystem::path& file, Context& context, bool isInGlobal) {
    if (!HandleIncludes(file, context, isInGlobal)) {
        return false;
    }

    return true;
}

bool SpirvGlslIncludeResolver::HandleIncludes(const std::filesystem::path& file, Context& context, bool isGlobal) {
    auto searchDirs{ context.includeDirectories };
    searchDirs.push_back(file.parent_path());

    std::regex reInclude{ "^#include ([\"<])(.*)([>\"])$" };
    std::sregex_iterator begin{ context.shaderSource.begin(), context.shaderSource.end(), reInclude };
    std::sregex_iterator end;

    u64 pos{};
    std::stringstream out;
    for (auto i{ begin }; i != end; ++i) {
        std::smatch matches{ *i };
        const auto isGlobalInclude{ matches.str(1) == "<" };

        if (!context.processGlobal && isGlobalInclude) {
            continue;
        }

        out << context.shaderSource.substr(pos, matches.position(0) - pos);

        const auto fileName{ matches.str(2) };
        auto file = [&]() -> std::optional<std::filesystem::path> {
            for (const auto& dir : searchDirs) {
                auto file{ dir / fileName };
                if (std::filesystem::exists(file)) {
                    return file;
                }
            }

            return std::nullopt;
        }();

        if (!file) {
            error_ = std::format("Failed to open file '{}' in ", fileName);
            for (const auto& dir : searchDirs) {
                error_ += std::format("{}, ", dir.string());
            }

            return false;
        }

        if (context.includedFiles) {
            context.includedFiles->insert(*file);
        }

        if (!IncludeFile(out, *file, context, isGlobalInclude || isGlobal)) {
            return false;
        }

        pos = matches.position(0) + matches.length(0);
    }

    if (pos > 0) {
        out << context.shaderSource.substr(pos, context.shaderSource.size() - pos);
        context.shaderSource = out.str();
    }

    return true;
}

bool SpirvGlslIncludeResolver::IncludeFile(std::ostream& out, const std::filesystem::path& path, Context& context, bool isGlobalInclude) {
    Context newContext{
        .shaderSource = ReadFile(path),
        .includeDirectories = context.includeDirectories,
        .includedFiles = context.includedFiles,
    };

    if (!ProcessImpl(path, newContext, isGlobalInclude)) {
        return false;
    }

    out << newContext.shaderSource;

    return true;
}

} // namespace ugine::gfxapi
