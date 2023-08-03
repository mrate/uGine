#include <format>
#include <fstream>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Missing input file\n";
        return -1;
    }

    std::ostream* out{ &std::cout };
    std::ofstream fout;
    if (argc > 2) {
        fout = std::ofstream{ argv[2] };
        out = &fout;
    }

    using namespace Assimp;

    Importer importer{};
    auto scene{ importer.ReadFile(argv[1], aiProcess_Triangulate) };

    if (scene->mNumMeshes < 1) {
        std::cerr << "No meshes found\n";
        return -1;
    }

    auto mesh{ scene->mMeshes[0] };

    (*out) << std::format("// {}\n", mesh->mNumFaces * 3);
    (*out) << std::format("const vec4 {}[] ={{\n", argc > 3 ? argv[3] : "MESH");
    for (uint32_t i{}; i < mesh->mNumFaces; ++i) {
        const auto face{ mesh->mFaces[i] };

        if (face.mNumIndices != 3) {
            std::cerr << "Mesh not triangulated\n";
            return -1;
        }

        for (int j{}; j < face.mNumIndices; ++j) {
            const auto vertex{ mesh->mVertices[face.mIndices[j]] };
            (*out) << std::format("\tvec4({:f}, {:f}, {:f}, 1.0)", float(vertex.x), float(vertex.y), float(vertex.z)).c_str();

            if (j < face.mNumIndices - 1 || i < mesh->mNumFaces - 1) {
                (*out) << ",";
            }
            (*out) << "\n";
        }
    }

    (*out) << "};";

    return 0;
}