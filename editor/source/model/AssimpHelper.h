#pragma once

#include <ugine/engine/gfx/asset/SerializedModel.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ugine::ed {

void LoadMaterial(
    const Path& baseDir, SerializedModelMaterial& rawMaterial, uint32_t mask, const aiScene* sourceScene, const aiMaterial* material, bool fixPath);

std::string FixName(const char* name);

glm::mat4 AssimpToGlm(const aiMatrix4x4& matrix);
glm::mat3 AssimpToGlm(const aiMatrix3x3& matrix);

inline glm::vec3 AssimpToGlm(const aiVector3D& vector) {
    return glm::vec3(vector.x, vector.y, vector.z);
}

inline glm::fquat AssimpToGlm(const aiQuaternion& quat) {
    return glm::fquat(quat.w, quat.x, quat.y, quat.z);
}

void AssimpDumper(const aiScene* scene);

} // namespace ugine::ed