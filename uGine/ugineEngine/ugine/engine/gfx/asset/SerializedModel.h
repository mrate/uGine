#pragma once

#include <ugine/engine/core/Resource.h>

#include <ugine/Image.h>
#include <ugine/Span.h>
#include <ugine/Utils.h>
#include <ugine/Vector.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ugine {

enum class SerializedTextureMapping {
    DiffuseMap = 0,
    EmissiveMap,
    NormalMap,
    HeightMap,
    AmbientMap,
    SpecularMap,
    AoMap,
    MetallicRoughnessMap,

    COUNT,
};

enum class SerializedColorMapping {
    AmbientColor = 0,
    DiffuseColor,
    SpecularColor,
    EmissiveColor,
    BaseColorFactor,

    COUNT,
};

enum class SerializedFloatMapping {
    Shininess = 0,
    MetallicFactor,
    RoughnessFactor,

    COUNT,
};

struct SerializedModelMaterial {
    std::string name;

    std::map<SerializedTextureMapping, Image> images;
    std::map<SerializedColorMapping, glm::vec4> colors;
    std::map<SerializedFloatMapping, f32> values;
};

struct SerializedSkin {
    glm::vec4 jointIndices{};
    glm::vec4 jointWeights{};
};

struct SerializedModel {
    static const int BONES_PER_VERTEX{ 4 };
    static const u32 INVALID_INDEX{ u32(-1) };

    struct Vertex {
        glm::vec3 pos{};
        glm::vec3 normal{};
        glm::vec3 tangent{};
        glm::vec2 tex{};
    };

    struct Bone {
        std::string name;
        glm::mat4 offsetMatrix{ 1.0f };
    };

    struct Mesh {
        std::string name;
        glm::mat4 transformation; // Root to mesh recursive transformation.
        u32 material{};
        u32 indexOffset{};
        u32 indexCount{};
        u32 vertexOffset{};
    };

    struct Node {
        std::string name;
        glm::mat4 transformation;
        std::vector<u32> children;
    };

    // Global data.
    glm::mat4 rootTransform;
    glm::mat4 globalInverseTransform;

    // Basic mesh data.
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::vector<Mesh> meshes;
    std::vector<SerializedModelMaterial> materials;
    std::vector<ResourceID> materialIds;
    std::vector<SerializedSkin> verticesSkinned;

    // Hierarchy data.
    std::vector<Node> nodes;

    // Armature data.
    std::vector<Bone> bones;
    std::map<std::string, u32> boneNameToIndex;

    glm::vec3 aabbMin{};
    glm::vec3 aabbMax{};
};

bool LoadModel(Span<const u8> in, SerializedModel& out);
bool SaveModel(const SerializedModel& in, Vector<u8>& out);

} // namespace ugine
