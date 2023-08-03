#pragma once

#include <gfxapi/Types.h>

#include <ugine/String.h>

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/gfx/Material.h>
#include <ugine/engine/math/Aabb.h>
#include <ugine/engine/math/Raycast.h>

#include <vector>

namespace ugine {

class Engine;

struct SerializedModel;

class Model final : public Resource {
public:
    inline static const ResourceType TYPE{ "Model" };

    static const u32 INVALID_INDEX{ u32(-1) };

    Model(ResourceManager& manager, const ResourceID& id);

    ~Model() { Unload(); }

    void DeleteSocket(const StringID& name);

    struct Mesh {
        u32 indexStart{};
        u32 indexCount{};
        u32 vertexOffset{};
        u32 materialIndex{};
        glm::mat4 transformation{ 1.0f };

        Vector<glm::vec3> vertices;
        Vector<u32> indices; // TODO: u32 / u16
    };

    struct Bone {
        String name;
        StringID id;
        glm::mat4 offsetMatrix;
        u32 parentBone{ INVALID_INDEX };
    };

    struct Node {
        String name;
        StringID id;
        glm::mat4 transform;
        Vector<u32> children;
        u32 boneIndex{ INVALID_INDEX };
    };

    struct Socket {
        String name;
        StringID id;
        u32 boneIndex{};
        glm::mat4 offsetTransformation{ 1.0f };
    };

    const AABB& BoundingBox() const { return aabb; }

    gfxapi::BufferHandle VertexBuffer() const { return vertexBuffer; }
    gfxapi::BufferHandle IndexBuffer() const { return indexBuffer; }
    gfxapi::BufferHandle SkinnedBuffer() const { return skinnedBuffer; }
    gfxapi::IndexType IndexType() const { return indexType; }

    u64 VertexBufferSize() const { return vertexBufferSize; }
    u32 VertexCount() const { return vertexCount; }

    const glm::mat4& GlobalInverseTransform() const { return globalInverseTransform; }
    const glm::mat4& RootTransform() const { return rootTransform; }

    const Vector<Mesh>& Meshes() const { return meshes; }
    const Vector<Node>& Nodes() const { return nodes; }
    const Vector<Bone>& Bones() const { return bones; }
    const Vector<Socket>& Sockets() const { return sockets; }
    const Vector<ResourceHandle<Material>>& Materials() const { return materials; }

    ResourceHandle<Material> GetMaterial(u32 slot) const;
    void SetMaterial(u32 slot, ResourceHandle<Material> material);

    void AddSocket(const Socket& socket) { sockets.PushBack(socket); }
    void UpdateSocket(u32 slot, const Socket& socket) { sockets[slot] = socket; }

private:
    void SetParentBone(u32 node, u32 parent);

    gfxapi::BufferHandle vertexBuffer;
    gfxapi::BufferHandle indexBuffer;
    gfxapi::BufferHandle skinnedBuffer;
    gfxapi::IndexType indexType{ gfxapi::IndexType::Uint16 };

    u64 vertexBufferSize{};
    u32 vertexCount{};

    glm::mat4 globalInverseTransform{ 1.0f };
    glm::mat4 rootTransform{ 1.0f };

    Vector<Mesh> meshes;
    Vector<Node> nodes;
    Vector<Bone> bones;
    Vector<Socket> sockets;
    Vector<ResourceHandle<Material>> materials;

    // AABB.
    AABB aabb{};

    bool HandleLoad(Span<const u8> data) override;
    bool HandleUnload() override;
};

// Model resource instance.
// Can override materials of model resource.
class ModelInstance : public EventEmittor {
public:
    struct ReadyEvent {
        bool ready{};
    };

    ModelInstance() = default;

    ModelInstance(ResourceHandle<Model> model) { SetModel(model); }

    ModelInstance(ResourceHandle<Model> model, Vector<ResourceHandle<Material>>&& materials) {
        SetModelPriv(model);
        SetMaterialsPriv(materials);
        SetReady(CheckReady());
    }

    ModelInstance(const ModelInstance& other) {
        SetModelPriv(other.model_);
        SetMaterialsPriv(other.materials_.Clone());
        SetReady(CheckReady());
    }

    ModelInstance& operator=(const ModelInstance& other) {
        SetModelPriv(other.model_);
        SetMaterialsPriv(other.materials_.Clone());
        SetReady(CheckReady());

        return *this;
    }

    ModelInstance(ModelInstance&& other) {
        other.DeregisterModelChanged();
        for (auto& material : other.materials_) {
            other.DeregisterMaterialChanged(material);
        }

        model_ = std::exchange(other.model_, {});
        materials_ = std::move(other.materials_);

        RegisterModelChanged();
        for (auto& material : materials_) {
            RegisterMaterialChanged(material);
        }

        SetReady(CheckReady());
    }

    ModelInstance& operator=(ModelInstance&& other) {
        other.DeregisterModelChanged();
        for (auto& material : other.materials_) {
            other.DeregisterMaterialChanged(material);
        }

        model_ = std::exchange(other.model_, {});
        materials_ = std::move(other.materials_);

        RegisterModelChanged();
        for (auto& material : materials_) {
            RegisterMaterialChanged(material);
        }

        SetReady(CheckReady());

        return *this;
    }

    ~ModelInstance() {
        DeregisterModelChanged();
        for (auto& material : materials_) {
            DeregisterMaterialChanged(material);
        }
    }

    void SetModel(ResourceHandle<Model> model) {
        SetModelPriv(model);
        SetReady(CheckReady());
    }

    UGINE_FORCE_INLINE bool Ready() const { return ready_; }

    UGINE_FORCE_INLINE bool HasBones() const { return !model_->Bones().Empty(); }

    const ResourceHandle<Model>& GetModel() const { return model_; }

    void ResetMaterials() {
        for (auto& material : materials_) {
            DeregisterMaterialChanged(material);
            material = {};
        }
        SetReady(CheckReady());
    }

    void SetMaterial(u32 slot, ResourceHandle<Material> material) {
        if (slot >= materials_.Size()) {
            UGINE_ASSERT(false);
            return;
        }

        SetMaterialPriv(slot, material);
        SetReady(CheckReady());
    }

    ResourceHandle<Material> GetMaterial(u32 slot) const {
        if (!model_) {
            return {};
        }

        return (slot < materials_.Size() && materials_[slot]) ? materials_[slot] : model_->GetMaterial(slot);
    }

    const Vector<ResourceHandle<Material>>& GetMaterials() const { return materials_; }

    friend bool operator==(const ModelInstance& a, const ModelInstance& b) {
        if (a.model_ != b.model_ || a.materials_.Size() != b.materials_.Size()) {
            return false;
        }

        for (u32 i{}; i < a.materials_.Size(); ++i) {
            if (a.materials_[i] != b.materials_[i]) {
                return false;
            }
        }

        return true;
    }

    friend bool operator!=(const ModelInstance& a, const ModelInstance& b) { return !(a == b); }

private:
    void SetModelPriv(const ResourceHandle<Model>& model) {
        DeregisterModelChanged();
        model_ = model;
        RegisterModelChanged();

        if (model_ && model_->Ready()) {
            materials_.Resize(model_->Materials().Size());
        }
    }

    void SetMaterialPriv(u32 slot, ResourceHandle<Material>& material) {
        DeregisterMaterialChanged(materials_[slot]);
        materials_[slot] = material;
        RegisterMaterialChanged(materials_[slot]);
    }

    void SetMaterialsPriv(Vector<ResourceHandle<Material>> materials) {
        for (auto& material : materials_) {
            DeregisterMaterialChanged(material);
        }
        materials_ = std::move(materials);
        for (auto& material : materials_) {
            RegisterMaterialChanged(material);
        }
    }

    void SetReady(bool ready) {
        if (ready != ready_) {
            ready_ = ready;
            Emit<ReadyEvent>({ ready_ });
        }
    }

    bool CheckReady() const {
        if (!model_ || !model_->Ready()) {
            return false;
        }

        for (auto& material : materials_) {
            if (material && !material->Ready()) {
                return false;
            }
        }

        return true;
    }

    void OnModelChanged(const Resource::StateChangedEvent& event) {
        if (event.newState == ResourceState::Loaded) {
            materials_.Resize(model_->Materials().Size());
            SetReady(CheckReady());
        } else {
            SetReady(false);
        }
    }

    void OnMaterialChanged(const Resource::StateChangedEvent& event) {
        if (event.newState == ResourceState::Loaded) {
            SetReady(CheckReady());
        } else {
            SetReady(false);
        }
    }

    UGINE_FORCE_INLINE void RegisterModelChanged() {
        if (model_) {
            listener_.Connect<&ModelInstance::OnModelChanged>(*model_.Get(), this);
        }
    }

    UGINE_FORCE_INLINE void DeregisterModelChanged() {
        if (model_) {
            listener_.Disconnect(*model_.Get(), this);
        }
    }

    UGINE_FORCE_INLINE void RegisterMaterialChanged(ResourceHandle<Material>& material) {
        if (material) {
            listener_.Connect<&ModelInstance::OnMaterialChanged>(*material.Get(), this);
        }
    }

    UGINE_FORCE_INLINE void DeregisterMaterialChanged(ResourceHandle<Material>& material) {
        if (material) {
            listener_.Disconnect(*material.Get());
        }
    }

    ResourceHandle<Model> model_;
    Vector<ResourceHandle<Material>> materials_;
    ResourceListener listener_;
    bool ready_{};
};

} // namespace ugine