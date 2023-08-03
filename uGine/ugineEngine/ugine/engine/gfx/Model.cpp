#pragma once

#include "Model.h"
#include "Material.h"

#include "asset/SerializedModel.h"

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/math/Math.h>

#include <ugine/Profile.h>

#include <glm/gtx/transform.hpp>

namespace ugine {

void Model::SetParentBone(u32 node, u32 parent) {
    if (nodes[node].boneIndex != Model::INVALID_INDEX) {
        bones[nodes[node].boneIndex].parentBone = parent;
        parent = nodes[node].boneIndex;
    }

    for (auto child : nodes[node].children) {
        SetParentBone(child, parent);
    }
}

bool Model::HandleLoad(Span<const u8> data) {
    PROFILE_EVENT();

    SerializedModel serializedModel{};
    if (!LoadModel(data, serializedModel)) {
        return false;
    }

    aabb = AABB{ serializedModel.aabbMin, serializedModel.aabbMax };

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    using namespace gfxapi;

    Vector<MaterialVertex> vertices;

    {
        vertexCount = u32(serializedModel.vertices.size());
        vertices.Reserve(vertexCount);

        for (const auto& vertex : serializedModel.vertices) {
            vertices.EmplaceBack(vertex.pos, vertex.normal, vertex.tangent, vertex.tex);
        };

        vertexBufferSize = sizeof(MaterialVertex) * vertices.Size();
        BufferDesc desc{
            .name = "ModelVertexData",
            .flags = BufferFlags::Vertex | BufferFlags::Storage,
            .size = vertexBufferSize,
        };
        vertexBuffer = state->device.CreateBuffer(desc, vertices.Data(), vertexBufferSize);
    }

    if (serializedModel.vertices.size() < 65536) {
        Vector<u16> indices(serializedModel.indices.size());
        for (u32 i{}; i < indices.Size(); ++i) {
            UGINE_ASSERT(serializedModel.indices[i] < 65536);
            indices[i] = u16(serializedModel.indices[i]);
        }

        indexBuffer = state->device.CreateIndexBuffer(indices);
        indexType = gfxapi::IndexType::Uint16;
    } else {
        indexBuffer = state->device.CreateIndexBuffer(serializedModel.indices);
        indexType = gfxapi::IndexType::Uint32;
    }

    meshes.Resize(serializedModel.meshes.size());
    for (u32 i{}; i < serializedModel.meshes.size(); ++i) {
        const auto& serializedMesh{ serializedModel.meshes[i] };

        meshes[i].indexCount = serializedMesh.indexCount;
        meshes[i].indexStart = serializedMesh.indexOffset;
        meshes[i].vertexOffset = serializedMesh.vertexOffset;
        meshes[i].transformation = serializedMesh.transformation;

        meshes[i].indices.Resize(meshes[i].indexCount);
        memcpy(meshes[i].indices.Data(), serializedModel.indices.data(), meshes[i].indexCount * sizeof(u32));
        meshes[i].vertices.Resize(vertices.Size());

        for (u32 v{}; v < meshes[i].vertices.Size(); ++v) {
            meshes[i].vertices[v] = vertices[v].position;
        }

        meshes[i].materialIndex = serializedMesh.material;
    }

    materials.Resize(serializedModel.materialIds.size());
    for (u32 i{}; i < serializedModel.materialIds.size(); ++i) {
        const auto& materialId{ serializedModel.materialIds[i] };

        materials[i] = Manager().Get<Material>(materialId);
        if (materials[i]) {
            AddDependency(materials[i].Get());
        } else {
            // TODO: Null material.
            UGINE_ASSERT("Invalid material");
        }
    }

    globalInverseTransform = serializedModel.globalInverseTransform;
    rootTransform = serializedModel.rootTransform;

    nodes.Reserve(serializedModel.nodes.size());
    for (const auto& node : serializedModel.nodes) {
        const auto it{ serializedModel.boneNameToIndex.find(node.name) };

        const u32 boneIndex{ it == serializedModel.boneNameToIndex.end() ? INVALID_INDEX : it->second };
        nodes.PushBack(Node{
            .name = node.name.c_str(),
            .id = StringID{ node.name.c_str() },
            .transform = node.transformation,
            .children = Vector<u32>{ node.children.data(), node.children.size(), Manager().GetAllocator() },
            .boneIndex = boneIndex,
        });
    }

    bones.Reserve(serializedModel.bones.size());
    for (const auto& bone : serializedModel.bones) {
        bones.PushBack(Bone{
            .name = bone.name.c_str(),
            .id = StringID{ bone.name.c_str() },
            .offsetMatrix = bone.offsetMatrix,
        });
    }

    if (!bones.Empty()) {
        SetParentBone(0, INVALID_INDEX);
    }

    if (!serializedModel.verticesSkinned.empty()) {
        const u32 bufferSize{ u32(sizeof(SerializedSkin) * serializedModel.verticesSkinned.size()) };
        BufferDesc desc{
            .name = "ModelSkinData",
            .flags = BufferFlags::Storage,
            .size = bufferSize,
        };
        skinnedBuffer = state->device.CreateBuffer(desc, serializedModel.verticesSkinned.data(), bufferSize);
    }

    return true;
}

ResourceHandle<Material> Model::GetMaterial(u32 slot) const {
    UGINE_ASSERT(slot < materials.Size());

    return materials[slot];
}

void Model::SetMaterial(u32 slot, ResourceHandle<Material> material) {
    UGINE_ASSERT(slot < materials.Size());

    if (materials[slot]) {
        RemoveDependency(materials[slot].Get());
    }

    materials[slot] = material;

    if (materials[slot]) {
        AddDependency(materials[slot].Get());
    }
}

bool Model::HandleUnload() {
    PROFILE_EVENT();

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    if (vertexBuffer) {
        state->device.DestroyBuffer(vertexBuffer);
        vertexBuffer = {};
    }

    if (indexBuffer) {
        state->device.DestroyBuffer(indexBuffer);
        indexBuffer = {};
    }

    if (skinnedBuffer) {
        state->device.DestroyBuffer(skinnedBuffer);
        skinnedBuffer = {};
    }

    meshes.Clear();
    nodes.Clear();
    bones.Clear();
    sockets.Clear();

    for (auto& material : materials) {
        if (material) {
            RemoveDependency(material.Get());
        }
    }
    materials.Clear();

    return true;
}

Model::Model(ResourceManager& manager, const ResourceID& id)
    : Resource{ manager, TYPE, id }
    , meshes{ manager.GetAllocator() }
    , nodes{ manager.GetAllocator() }
    , bones{ manager.GetAllocator() }
    , sockets{ manager.GetAllocator() } {
}

void Model::DeleteSocket(const StringID& name) {
    sockets.EraseIf([&](const auto& socket) { return socket.id == name; });
}

} // namespace ugine