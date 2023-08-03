#include "MeshImporter.h"

#include "AssimpHelper.h"

#include <ugine/Error.h>
#include <ugine/Log.h>

#include <algorithm>

namespace ugine::ed {

namespace {
    inline float CalcTime(double time, const aiAnimation* anim) {
        return static_cast<float>(time / static_cast<float>(anim->mTicksPerSecond ? anim->mTicksPerSecond : 25.0f));
    }
} // namespace

MeshImporter::MeshImporter(const Path& file, const Settings& settings)
    : file_{ file }
    , settings_{ settings } {
    UGINE_TRACE("Loading mesh '{}'", file_.String());

    unsigned int flags{};
    flags |= aiProcess_CalcTangentSpace;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_SortByPType;
    flags |= aiProcess_GenUVCoords;
    //flags |= aiProcess_OptimizeMeshes;
    flags |= aiProcess_Debone;
    flags |= aiProcess_ValidateDataStructure;
    flags |= aiProcess_FlipUVs;
    flags |= aiProcess_GenNormals;
    //flags |= aiProcess_GenBoundingBoxes;

    importer_ = std::make_unique<Assimp::Importer>();
    scene_ = importer_->ReadFile(file.Data(), flags);

    if (!scene_ || !scene_->mRootNode) {
        UGINE_THROW(Error, importer_->GetErrorString());
    }
}

MeshImporter::~MeshImporter() {
}

Vector<SerializedModel> MeshImporter::LoadMeshes() const {
    Vector<SerializedModel> resultList;

    if (scene_->mNumMeshes == 0) {
        return resultList;
    }

    SerializedModel result{};
    result.rootTransform = settings_.transformation;
    result.globalInverseTransform = glm::inverse(AssimpToGlm(scene_->mRootNode->mTransformation));

    // Load scene tree.
    meshTransformations_.Resize(scene_->mNumMeshes);
    result.nodes.push_back({});
    result.nodes[0] = PopulateScene(result.nodes, scene_->mRootNode, glm::mat4{ 1.0f });

    const auto dir{ file_.ParentPath() };
    if (settings_.singleMesh) {
        uint32_t numVertices{};
        uint32_t numIndices{};

        for (uint32_t i{}; i < scene_->mNumMeshes; ++i) {
            const auto mesh{ scene_->mMeshes[i] };

            SerializedModel::Mesh newMesh{};
            newMesh.name = mesh->mName.C_Str();
            newMesh.transformation = settings_.transformation * meshTransformations_[i];
            newMesh.vertexOffset = numVertices;
            newMesh.indexOffset = numIndices;
            newMesh.indexCount = mesh->mNumFaces * 3;

            numVertices += mesh->mNumVertices;
            numIndices += mesh->mNumFaces * 3;

            result.meshes.push_back(std::move(newMesh));
        }

        //// TODO:
        //if (scene_->mNumMeshes > 0) {
        //    result.globalInverseTransform = glm::inverse(meshTransformations_[0]);
        //}

        for (uint32_t i = 0; i < scene_->mNumMeshes; ++i) {
            LoadMesh(dir, scene_->mMeshes[i], result, i);
        }

        CalcAabb(result);

        resultList.PushBack(result);
    } else {
        for (uint32_t i = 0; i < scene_->mNumMeshes; ++i) {
            SerializedModel importMesh;
            const auto mesh{ scene_->mMeshes[i] };

            SerializedModel::Mesh newMesh{};
            newMesh.name = mesh->mName.C_Str();
            newMesh.transformation = settings_.transformation * meshTransformations_[i];
            newMesh.vertexOffset = 0;
            newMesh.indexOffset = 0;
            newMesh.indexCount = mesh->mNumFaces * 3;

            importMesh.meshes.push_back(std::move(newMesh));

            LoadMesh(dir, scene_->mMeshes[i], importMesh, 0);
            materialMap_.clear();

            CalcAabb(importMesh);

            resultList.PushBack(importMesh);
        }
    }

    return resultList;
}

Vector<SerializedAnimation> MeshImporter::LoadAnimations() const {
    Vector<SerializedAnimation> result;
    LoadAnimations(result);
    return result;
}

void MeshImporter::CalcAabb(SerializedModel& model) const {
    model.aabbMin = model.aabbMax = glm::vec3{};

    for (const auto& mesh : model.meshes) {
        for (uint32_t i{ 0 }; i < mesh.indexCount; ++i) {
            const auto index{ model.indices[mesh.indexOffset + i] };
            const auto& vertex{ model.vertices[mesh.vertexOffset + index] };

            const auto pos{ glm::vec3{ mesh.transformation * glm::vec4{ vertex.pos, 1.0f } } };

            model.aabbMin = glm::min(model.aabbMin, pos);
            model.aabbMax = glm::max(model.aabbMax, pos);
        }
    }
}

void MeshImporter::LoadMesh(const Path& dir, const aiMesh* sourceMesh, SerializedModel& model, uint32_t meshIndex) const {
    // Load vertices and indices.
    model.aabbMin = model.aabbMax = glm::vec3{};

    for (uint32_t i = 0; i < sourceMesh->mNumVertices; ++i) {
        SerializedModel::Vertex vertex{};
        vertex.pos.x = sourceMesh->mVertices[i].x;
        vertex.pos.y = sourceMesh->mVertices[i].y;
        vertex.pos.z = sourceMesh->mVertices[i].z;

        if (sourceMesh->HasNormals()) {
            vertex.normal.x = sourceMesh->mNormals[i].x;
            vertex.normal.y = sourceMesh->mNormals[i].y;
            vertex.normal.z = sourceMesh->mNormals[i].z;
        }

        if (sourceMesh->HasTextureCoords(0)) {
            vertex.tex.x = sourceMesh->mTextureCoords[0][i].x;
            vertex.tex.y = sourceMesh->mTextureCoords[0][i].y;
        } else {
            vertex.tex = { 0, 0 };
        }

        if (sourceMesh->HasTangentsAndBitangents()) {
            vertex.tangent = { sourceMesh->mTangents[i].x, sourceMesh->mTangents[i].y, sourceMesh->mTangents[i].z };
        } else {
            vertex.tangent = { 0, 0, 0 };
        }

        model.vertices.push_back(vertex);
        model.verticesSkinned.emplace_back();

        UGINE_ASSERT(model.vertices.size() == model.verticesSkinned.size());
    }

    // Process indices.
    for (uint32_t i{}; i < sourceMesh->mNumFaces; ++i) {
        aiFace face = sourceMesh->mFaces[i];
        assert(face.mNumIndices == 3);
        for (uint32_t j{}; j < face.mNumIndices; ++j) {
            model.indices.push_back(face.mIndices[j]);
        }
    }

    // Import material.
    if (sourceMesh->mMaterialIndex >= 0) {
        auto it{ materialMap_.find(sourceMesh->mMaterialIndex) };
        if (it == materialMap_.end()) {
            it = materialMap_.insert(std::make_pair(sourceMesh->mMaterialIndex, static_cast<uint32_t>(model.materials.size()))).first;

            aiMaterial* material{ scene_->mMaterials[sourceMesh->mMaterialIndex] };

            SerializedModelMaterial mat{};
            mat.name = material->GetName().C_Str();
            LoadMaterial(dir, mat, settings_.textureMask, scene_, material, true);
            model.materials.push_back(std::move(mat));
        }
        model.meshes[meshIndex].material = it->second;
    }

    // Load bones.
    for (uint32_t i{}; i < sourceMesh->mNumBones; ++i) {
        const auto sourceBone{ sourceMesh->mBones[i] };
        const auto boneName{ settings_.fixPaths ? FixName(sourceBone->mName.C_Str()) : sourceBone->mName.C_Str() };

        const auto it{ model.boneNameToIndex.find(boneName) };

        uint32_t boneIndex{};
        if (it == model.boneNameToIndex.end()) {
            SerializedModel::Bone bone{};

            bone.name = boneName;
            bone.offsetMatrix = AssimpToGlm(sourceBone->mOffsetMatrix);

            boneIndex = static_cast<uint32_t>(model.bones.size());
            model.bones.push_back(bone);

            model.boneNameToIndex.insert(std::make_pair(boneName, boneIndex));
        } else {
            boneIndex = it->second;
        }

        for (uint32_t j{}; j < sourceBone->mNumWeights; ++j) {
            const auto weight{ sourceBone->mWeights[j] };
            const uint32_t vertexId{ model.meshes[meshIndex].vertexOffset + weight.mVertexId };

            AddVertexWeight(model, vertexId, boneIndex, weight.mWeight);
        }
    }
}

void MeshImporter::AddVertexWeight(SerializedModel& mesh, uint32_t vertexIndex, uint32_t boneIndex, float boneWeight) const {
    UGINE_ASSERT(mesh.vertices.size() > vertexIndex);
    UGINE_ASSERT(mesh.verticesSkinned.size() > vertexIndex);
    UGINE_ASSERT(mesh.bones.size() > boneIndex);

    auto& vertex{ mesh.verticesSkinned[vertexIndex] };

    for (int i{}; i < SerializedModel::BONES_PER_VERTEX; ++i) {
        if (vertex.jointWeights[i] == 0.0f) {
            vertex.jointIndices[i] = static_cast<float>(boneIndex);
            vertex.jointWeights[i] = boneWeight;

            if (i == SerializedModel::BONES_PER_VERTEX - 1) {
                float sum{};
                for (int j{}; j < SerializedModel::BONES_PER_VERTEX; ++j) {
                    sum += vertex.jointWeights[j];
                }

                // Normalize so that sum is always 1.
                // TODO: What about vertices with bone indices < BONES_PER_VERTEX?
                for (int j{}; j < SerializedModel::BONES_PER_VERTEX; ++j) {
                    vertex.jointWeights[j] /= sum;
                }
            }

            return;
        }
    }

    UGINE_ASSERT("Bone per vertex limit reached.");
}

uint32_t MeshImporter::BoneIndex(SerializedModel& mesh, const std::string& name) const {
    auto it{ mesh.boneNameToIndex.find(name) };
    UGINE_ASSERT(it != mesh.boneNameToIndex.end());
    return it->second;
}

SerializedModel::Node MeshImporter::PopulateScene(std::vector<SerializedModel::Node>& nodes, const aiNode* node, const glm::mat4& parentTransform) const {
    SerializedModel::Node newNode{};
    newNode.name = FixName(node->mName.C_Str());
    newNode.transformation = AssimpToGlm(node->mTransformation);

    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        meshTransformations_[node->mMeshes[i]] = parentTransform * newNode.transformation;
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        const auto index{ nodes.size() };
        newNode.children.push_back(static_cast<unsigned int>(index));
        nodes.push_back({});
        nodes[index] = PopulateScene(nodes, node->mChildren[i], parentTransform * newNode.transformation);
    }

    return newNode;
}

void MeshImporter::LoadAnimations(Vector<SerializedAnimation>& animations) const {
    for (uint32_t i = 0; i < scene_->mNumAnimations; ++i) {
        auto animation{ scene_->mAnimations[i] };

        SerializedAnimation anim{};
        anim.name = animation->mName.C_Str();
        if (anim.name.empty()) {
            anim.name = std::string("animation#") + std::to_string(animations.Size());
        }

        anim.lengthSeconds = CalcTime(animation->mDuration, animation);

        for (uint32_t c = 0; c < animation->mNumChannels; ++c) {
            const auto channel{ animation->mChannels[c] };

            SerializedAnimation::Channel newChannel{};

            std::string nodeName{ FixName(channel->mNodeName.C_Str()) };

            for (uint32_t t = 0; t < channel->mNumPositionKeys; ++t) {
                const auto position{ channel->mPositionKeys[t] };
                const auto time{ CalcTime(position.mTime, animation) };
                const auto posVec{ AssimpToGlm(position.mValue) };
                newChannel.positions.push_back(std::make_pair(time, posVec));
            }

            for (uint32_t t = 0; t < channel->mNumRotationKeys; ++t) {
                const auto rotation{ channel->mRotationKeys[t] };
                const auto time{ CalcTime(rotation.mTime, animation) };
                const auto rotQuat{ AssimpToGlm(rotation.mValue) };
                newChannel.rotations.push_back(std::make_pair(time, rotQuat));
            }

            for (uint32_t t = 0; t < channel->mNumScalingKeys; ++t) {
                const auto scale{ channel->mScalingKeys[t] };
                const auto time{ CalcTime(scale.mTime, animation) };
                const auto scaleVec{ AssimpToGlm(scale.mValue) };
                newChannel.scales.push_back(std::make_pair(time, scaleVec));
            }

            anim.channels.insert(std::make_pair(nodeName, newChannel));
        }

        animations.PushBack(anim);
    }
}

} // namespace ugine::ed