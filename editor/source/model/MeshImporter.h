#pragma once

#include <ugine/engine/gfx/asset/SerializedAnimation.h>
#include <ugine/engine/gfx/asset/SerializedModel.h>
#include <ugine/engine/math/Math.h>

#include <ugine/Path.h>
#include <ugine/Vector.h>

#include <map>

struct aiMesh;
struct aiNode;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
class Importer;
}

namespace ugine::ed {

class MeshImporter {
public:
    struct Settings {
        glm::mat4 transformation{ 1.0f };
        bool fixPaths{ true };
        bool singleMesh{ false };
        uint32_t textureMask{ 0xffffffff };
    };

    MeshImporter(const Path& file, const Settings& settings = {});
    ~MeshImporter();

    Vector<SerializedModel> LoadMeshes() const;
    Vector<SerializedAnimation> LoadAnimations() const;

private:
    void CalcAabb(SerializedModel& model) const;
    void LoadMesh(const Path& dir, const aiMesh* sourceMesh, SerializedModel& mesh, uint32_t index) const;
    void LoadAnimations(Vector<SerializedAnimation>& animations) const;

    void AddVertexWeight(SerializedModel& mesh, uint32_t vertexIndex, uint32_t boneIndex, float boneWeight) const;

    uint32_t BoneIndex(SerializedModel& mesh, const std::string& name) const;

    SerializedModel::Node PopulateScene(std::vector<SerializedModel::Node>& nodes, const aiNode* node, const glm::mat4& parentTransform) const;

    Path file_;
    mutable Settings settings_;

    std::unique_ptr<Assimp::Importer> importer_;
    const aiScene* scene_{};

    mutable std::map<uint32_t, uint32_t> materialMap_;
    mutable Vector<glm::mat4> meshTransformations_;
};

} // namespace ugine::ed