#pragma once

namespace ugine::ed {

class LodGenerator {
public:
    // TODO:

    // Generate lods.
    //for (auto& mesh : result.meshes) {
    //    std::vector<IndexType> indices(result.indices.begin() + mesh.lod[0].baseIndex, result.indices.begin() + mesh.lod[0].baseIndex + mesh.lod[0].numIndices);
    //    MeshOptimizer optimizer{ result.vertices, indices };
    //    float lod{ 0.5f };
    //    for (int i = 0; i < lodLevels - 1; ++i) {
    //        std::vector<IndexType> newIndices;
    //        optimizer.CreateLod(lod, newIndices);

    //        if (newIndices.size() == mesh.lod[i].numIndices) {
    //            break;
    //        }

    //        ugine::SerializedModel::MeshLod meshLod{ static_cast<uint32_t>(result.indices.size()), static_cast<uint32_t>(newIndices.size()) };
    //        mesh.lod.push_back(meshLod);

    //        std::copy(newIndices.begin(), newIndices.end(), std::back_inserter(result.indices));

    //        lod *= 0.5f;
    //    }
    //}
};

} // namespace ugine::ed