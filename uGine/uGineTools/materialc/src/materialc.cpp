#include <MaterialImporter.h>

#include <uGine/Vector.h>
#include <ugine/File.h>
#include <ugine/Path.h>
#include <ugine/StringUtils.h>

#include <ugine/engine/gfx/asset/SerializedMaterial.h>

#include <format>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage " << argv[0] << " [material_file] [output_file] [[include_dir] ...]"
                  << "\n";
        return -1;
    }

    const ugine::Path inputFile{ argv[1] };
    const ugine::Path outputFile{ argv[2] };
    const ugine::Path outputDir{ outputFile.ParentPath() };

    ugine::Vector<ugine::Path> includeDirs;
    for (int i{ 3 }; i < argc; ++i) {
        includeDirs.EmplaceBack(argv[i]);
    }

    std::cout << std::format("Building material '{}' => '{}'...", inputFile.String(), outputFile.String()) << std::endl;

    if (!ugine::FileSystem::Exists(outputDir)) {
        ugine::FileSystem::CreateDirectories(outputDir);
    }

    ugine::tools::MaterialImporter importer{ inputFile };
    try {
        auto serialized{ importer.LoadMaterial(includeDirs) };

        ugine::Vector<u8> out;
        ugine::SaveMaterial(serialized, out);

        ugine::WriteFileBinary(outputFile, out.ToSpan());
    } catch (const std::exception& ex) {
        std::cerr << "Failed to build material: " << ex.what() << std::endl;

        for (const auto& error : importer.GetErrors()) {
            std::cerr << std::format("{} at {}:{}\n", error.message, error.filename.String(), 0);
            //std::cerr << error.shaderProcessedSource << std::endl;
        }

        return -1;
    }

    //std::cout << "Material built" << std::endl;
    return 0;
}