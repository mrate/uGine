#include "AssimpHelper.h"

#include <ugine/Log.h>

#include <assimp/material.h>

#include <iostream>

namespace ugine::ed {

bool LoadMaterialTexture(
    const Path& path, const aiScene* scene, const aiMaterial* mat, aiTextureType type, unsigned int index, bool generateMips, bool fixPath, Image& tex) {

    if (index >= mat->GetTextureCount(type)) {
        return false;
    }

    aiString str;
    mat->GetTexture(type, index, &str);

    auto embeddedTexture{ scene->GetEmbeddedTexture(str.C_Str()) };
    if (embeddedTexture || str.data[0] == '*') {
        int index = atoi(str.data + 1);
        int width{ static_cast<int>(embeddedTexture ? embeddedTexture->mWidth : scene->mTextures[index]->mWidth) };
        int height{ static_cast<int>(embeddedTexture ? embeddedTexture->mHeight : scene->mTextures[index]->mHeight) };
        auto* data{ embeddedTexture ? embeddedTexture->pcData : scene->mTextures[index]->pcData };

        //vk::Extent3D extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

        if (height == 0) {
            // Width is the size of embedded image data.
            if (!Image::FromMemoryEncoded(reinterpret_cast<unsigned char*>(data), width, tex)) {
                return false;
            }
        } else {
            int pixel = 0;
            std::vector<unsigned char> rgba(width * height * 4);
            for (int j = 0, size = width * height; j < size; j += 4) {
                rgba[pixel++] = data[j].r;
                rgba[pixel++] = data[j].g;
                rgba[pixel++] = data[j].b;
                rgba[pixel++] = data[j].a;
            }
            if (!Image::FromMemoryDecoded(width, height, Span<const u8>{ rgba.data(), rgba.size() }, tex)) {
                return false;
            }
        }
    } else {
        auto file{ path / str.C_Str() };

        if (!FileSystem::Exists(file) && fixPath) {
            Path textureFile{ str.C_Str() };
            file = path / textureFile.Filename();
        }

        if (!Image::FromFile(file, tex)) {
            return false;
        }
    }

    return true;
}

std::string FixName(const char* name) {
    // TODO: Mixamo hack.
    std::string res{ name };
    const auto pos{ res.find("mixamorig:") };
    return pos != std::string::npos ? res.substr(10, res.size() - 10) : res;
}

glm::vec4 LoadColor(const aiMaterial* material, const char* pKey, unsigned int type, unsigned int idx, const glm::vec4& def) {
    aiColor4D color;

    if (material->Get(pKey, type, idx, color) == aiReturn_SUCCESS) {
        return glm::vec4{ color.r, color.g, color.b, color.a };
    }

    return def;
}

void LoadMaterial(
    const Path& baseDir, SerializedModelMaterial& rawMaterial, uint32_t mask, const aiScene* sourceScene, const aiMaterial* material, bool fixPath) {
    //rawMaterial.diffuseMap = LoadMaterialTexture(baseDir, sourceScene, material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, true);
    //rawMaterial.metallicRoughnessMap
    //    = LoadMaterialTexture(baseDir, sourceScene, material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, true, fixPath);
    const aiTextureType types[] = {
        aiTextureType_DIFFUSE,
        aiTextureType_EMISSIVE,
        aiTextureType_NORMALS,
        aiTextureType_HEIGHT,
        aiTextureType_AMBIENT,
        aiTextureType_SPECULAR,
        aiTextureType_LIGHTMAP,
        aiTextureType_LIGHTMAP,
    };
    // TODO: MetallicRoughnessMap,

    for (uint32_t i{}; i < uint32_t(SerializedTextureMapping::COUNT); ++i) {
        if ((mask & (1 << i)) == 0) {
            continue;
        }

        Image image{};
        auto res{ LoadMaterialTexture(baseDir, sourceScene, material, types[i], 0, true, fixPath, image) };
        if (res) {
            rawMaterial.images[SerializedTextureMapping(i)] = std::move(image);
        }
    }

    rawMaterial.colors[SerializedColorMapping::AmbientColor] = LoadColor(material, AI_MATKEY_COLOR_AMBIENT, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    rawMaterial.colors[SerializedColorMapping::DiffuseColor] = LoadColor(material, AI_MATKEY_COLOR_DIFFUSE, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    rawMaterial.colors[SerializedColorMapping::SpecularColor] = LoadColor(material, AI_MATKEY_COLOR_SPECULAR, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
    rawMaterial.colors[SerializedColorMapping::EmissiveColor] = LoadColor(material, AI_MATKEY_COLOR_EMISSIVE, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    //rawMaterial.baseColorFactor = LoadColor(material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, rawMaterial.diffuseColor);

    // ???
    float value{};
    if (material->Get(AI_MATKEY_SHININESS, value) != aiReturn_SUCCESS) {
        rawMaterial.values[SerializedFloatMapping::Shininess] = 20.0f;
    } else {
        rawMaterial.values[SerializedFloatMapping::Shininess] = value;
    }

    //rawMaterial.metallicFactor = 1.0f;
    //if (material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, rawMaterial.metallicFactor) != aiReturn_SUCCESS) {
    //    auto res{ material->Get(AI_MATKEY_REFLECTIVITY, rawMaterial.metallicFactor) };
    //}

    //if (material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, rawMaterial.roughnessFactor) != aiReturn_SUCCESS) {
    //    rawMaterial.roughnessFactor = 1.0f;
    //}
}

glm::mat4 AssimpToGlm(const aiMatrix4x4& matrix) {
    // TODO: Unpack + inline.
    glm::mat4 result;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result[col][row] = matrix[row][col];
        }
    }
    return result;
}

glm::mat3 AssimpToGlm(const aiMatrix3x3& matrix) {
    // TODO: Unpack + inline.
    glm::mat3 result;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result[col][row] = matrix[row][col];
        }
    }
    return result;
}

void DumpNode(const aiNode* node, uint32_t depth) {
    for (uint32_t i = 0; i < depth; ++i) {
        std::cout << "--";
    }

    std::cout << " " << node->mName.C_Str() << " " << node->mNumMeshes << " meshes, " << node->mNumChildren << " children" << std::endl;

    for (uint32_t i = 0; i < depth; ++i) {
        std::cout << "--";
    }
    std::cout << " [" << node->mNumMeshes << " meshes: ";
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        std::cout << node->mMeshes[i] << ", ";
    }
    std::cout << "]" << std::endl;

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        DumpNode(node->mChildren[i], depth + 1);
    }
}

void AssimpDumper(const aiScene* scene) {
    std::cout << "    meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "animations: " << scene->mNumAnimations << std::endl;
    std::cout << " materials: " << scene->mNumMaterials << std::endl;
    std::cout << " Node tree:" << std::endl;
    DumpNode(scene->mRootNode, 0);

    std::cout << std::endl << "Meshes:" << std::endl;
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        std::cout << i << ": " << scene->mMeshes[i]->mName.C_Str() << std::endl;
        std::cout << "  Bones:" << std::endl;
        for (uint32_t j = 0; j < scene->mMeshes[i]->mNumBones; ++j) {
            std::cout << "   " << j << ": " << scene->mMeshes[i]->mBones[j]->mName.C_Str() << std::endl;
        }
    }

    std::cout << std::endl << "Animations: " << std::endl;
    for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        std::cout << i << ": " << scene->mAnimations[i]->mName.C_Str() << std::endl;
        std::cout << "  Channels: " << std::endl;
        for (uint32_t j = 0; j < scene->mAnimations[i]->mNumChannels; ++j) {
            std::cout << "   - " << scene->mAnimations[i]->mChannels[j]->mNodeName.C_Str() << std::endl;
        }
    }
}

} // namespace ugine::ed