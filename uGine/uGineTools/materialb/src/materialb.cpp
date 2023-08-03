#include <MaterialBuilder.h>

#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage " << argv[0] << " [material_file] [output_file] [[include_dir] ...]"
                  << "\n";
        return -1;
    }

    const std::filesystem::path inputFile{ argv[1] };
    const std::filesystem::path outputFile{ argv[2] };
    const std::filesystem::path outputDir{ outputFile.parent_path() };

    std::vector<std::filesystem::path> includeDirs;
    for (int i{ 3 }; i < argc; ++i) {
        includeDirs.push_back(argv[i]);
    }

    std::cout << std::format("Building high-level material '{}' => '{}'...\n", inputFile.string(), outputFile.string());

    try {
        ugine::tools::MaterialBuilder builder{ "name", inputFile };

        {
            std::ofstream out{ outputFile };
            out << builder.Descriptor();
        }

        for (const auto& [shader, source] : builder.Shaders()) {
            std::ofstream out{ outputDir / shader };
            out << source;
        }

    } catch (const std::exception& ex) {
        std::cerr << std::format("Error: {}\n", ex.what());
        return -1;
    }

    return 0;
}