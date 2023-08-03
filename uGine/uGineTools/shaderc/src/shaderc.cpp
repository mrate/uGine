#include <ShaderImporter.h>

#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct ArgContext {
    int position;
    int argc;
    char** argv;

    bool Ready() const { return position < argc; }
    const char* Arg() const { return argv[position]; }
};

template <typename T> T ParseArg(const char* str);

template <> std::string ParseArg(const char* str) {
    return str;
}

template <typename T> std::optional<T> ReadArgument(std::string_view name, ArgContext& context) {
    if (strcmp(context.argv[context.position], name.data()) == 0) {
        if (context.position < context.argc - 1) {
            return ParseArg<T>(context.argv[(++context.position)++]);
        } else {
            throw std::runtime_error{ std::format("Missing argument for '{}'", name).c_str() };
        }
    }

    return std::nullopt;
}

template <> std::optional<bool> ReadArgument(std::string_view name, ArgContext& context) {
    if (context.position < context.argc && strcmp(context.argv[context.position], name.data()) == 0) {
        ++context.position;
        return true;
    }
    return std::nullopt;
}

bool ParseOptions(int argc, char* argv[], ShaderImporter::Options& options) {
    ArgContext context{ 3, argc, argv };
    while (context.Ready()) {
        if (auto includeDir = ReadArgument<std::string>("-I", context)) {
            options.includeDirs.push_back(std::filesystem::canonical(*includeDir));
            continue;
        }

        if (auto warn = ReadArgument<bool>("-W", context)) {
            options.warningsAsErrors = true;
            continue;
        }

        if (auto optimize = ReadArgument<bool>("-O", context)) {
            options.optimizeForPerf = true;
            continue;
        }

        if (auto debug = ReadArgument<bool>("-G", context)) {
            options.generateDebugInfo = true;
            continue;
        }

        if (auto debug = ReadArgument<bool>("-V", context)) {
            options.verbose = true;
            continue;
        }

        if (auto intermediateDir = ReadArgument<std::string>("-H", context)) {
            options.intermediateDir = *intermediateDir;
            continue;
        }

        if (auto saveSteps = ReadArgument<bool>("-S", context)) {
            options.saveSteps = true;
            continue;
        }

        std::cerr << std::format("Unknown argument '{}'\n", context.Arg());
        return false;
    }

    return true;
}

inline void Usage(const char* name) {
    std::cerr << "Usage " << name << " <shader_source> <shader_output> [-I include_dir] [-W] [-O] [-G] [-V]"
              << "\n";
    std::cerr << "\t-I include_dir\tAdd include search path.\n";
    std::cerr << "\t-D define\tDeclare predefined macro.\n";
    std::cerr << "\t-H intermediate_dir\tDirectory for intermediate result (not used if empty).\n";
    std::cerr << "\t-W \t\tWarnings as errors.\n";
    std::cerr << "\t-O \t\tOptimize for performance.\n";
    std::cerr << "\t-G \t\tAdd debug info.\n";
    std::cerr << "\t-V \t\tVerbose.\n";
    std::cerr << "\t-S \t\tSave steps.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        ::Usage(argv[0]);
        return -1;
    }

    try {
        ShaderImporter::Options options{};
        options.inputFile = argv[1];
        options.outputFile = argv[2];

        if (!ParseOptions(argc, argv, options)) {
            ::Usage(argv[0]);
            return -1;
        }

        if (options.verbose) {
            std::cout << std::format("Compiling shader '{}' => '{}'...\n", options.inputFile.string(), options.outputFile.string());
        }

        ShaderImporter importer{};
        if (!importer.Import(options)) {
            if (options.verbose) {
                std::cerr << importer.Error() << "\n";

                // TODO: Output shader errors.
            }

            return -1;
        }

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return -1;
    }

    return 0;
}