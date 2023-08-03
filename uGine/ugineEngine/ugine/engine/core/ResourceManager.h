#pragma once

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/core/ResourceStorage.h>

#include <ugine/Path.h>
#include <ugine/Log.h>

#include <unordered_map>

namespace ugine {

class Engine;

class ResourceManager {
public:
    struct ResourceRef {
        Path path;
        String name;
        ResourceType type;
    };

    explicit ResourceManager(Engine& engine, IAllocator& allocator)
        : engine_{ engine }
        , allocator_{ allocator } {}

    ~ResourceManager() {}

    IAllocator& GetAllocator() const { return allocator_; }
    Engine& GetEngine() const { return engine_; }
    //FileSystem& GetFileSystem() const { return engine_.GetFileSystem(); }

    template <typename T> ResourceHandle<T> Create() {
        // TODO: Locking.

        if (auto storage = GetStorage<T>()) {
            return storage->Create(*this, ResourceID::Generate());
        }

        UGINE_ASSERT(false && "Unknown resource type");
        return ResourceHandle<T>{};
    }

    template <typename T> void Delete(const ResourceID& id) { Delete(id, T::TYPE); }

    void Delete(const ResourceID& id, const ResourceType& type) {
        // TODO: Locking.

        if (auto storage = GetStorage(type)) {
            storage->Delete(id);
        }

        DeregisterResource(id);
    }

    template <typename T> void Clear() {
        // TODO: Locking.

        if (auto storage = GetStorage<T>()) {
            for (auto& [id, _] : storage->All()) {
                DeregisterResource(id);
            }

            storage->Clear();
        }
    }

    template <typename T> void Reload(const ResourceID& id) {
        // TODO: Locking.

        // TODO: Only mark for reload, make sure resource state change happens in one particular timepoint in frame (Sync() ?)

        auto storage = GetStorage<T>();
        if (!storage) {
            return;
        }

        auto handle{ storage->Get(id) };
        if (!handle) {
            return;
        }

        if (handle->State() == ResourceState::Unloaded) {
            // Don't bother reloading unloaded resources => no one is using them anyway.
            return;
        }

        handle->Unload();
        auto ref{ resourcesById_.find(id) };
        if (ref != resourcesById_.end()) {
            handle->LoadAsync(ref->second.path.String());
        }
    }

    template <typename T> ResourceHandle<T> Get(const ResourceID& id) {
        // TODO: Locking.

        auto storage = GetStorage<T>();
        if (!storage) {
            UGINE_ASSERT(false && "Unknown resource type");
            return {};
        }

        auto handle{ storage->Get(id) };
        if (handle) {
            if (handle->State() == ResourceState::Unloaded) {
                // Resource exists but it's not loaded -> load it.
                auto ref{ resourcesById_.find(id) };
                if (ref != resourcesById_.end()) {
                    handle->LoadAsync(ref->second.path.String());
                }
            }

            return handle;
        }

        auto ref{ resourcesById_.find(id) };
        if (ref == resourcesById_.end()) {
            // Resource ID not registered.
            return {};
        }

        // Resource doesn't exist, create & load.
        handle = storage->Create(*this, id);
        if (!ref->second.path.Empty()) {
            handle->LoadAsync(ref->second.path.String());
        }
        return handle;
    }

    template <typename T> const std::unordered_map<ResourceID, u64>& All() const {
        // TODO: Locking.

        if (auto storage = GetStorage<T>()) {
            return storage->All();
        }

        return NullMap;
    }

    const std::unordered_map<ResourceID, u64>& All(const ResourceType& type) const {
        // TODO: Locking.

        if (auto storage = GetStorage(type)) {
            return storage->All();
        }

        static std::unordered_map<ResourceID, u64> null;
        return NullMap;
    }

    void RegisterResource(const ResourceID& id, const Path& path, const ResourceType& type) {
        // TODO: Locking.

        UGINE_ASSERT(!resourcesById_.contains(id));
        UGINE_ASSERT(!resourcesByPath_.contains(path));

        resourcesById_.insert(std::make_pair(id,
            ResourceRef{
                .path = path,
                .name = path.Filename(),
                .type = type,
            }));
        resourcesByPath_.insert(std::make_pair(path, id));

        UGINE_ASSERT(resourcesById_.size() == resourcesByPath_.size());

        auto storage{ storages_.find(type) };
        if (storage != storages_.end()) {
            storage->second->Create(*this, id);
        }
    }

    void DeregisterResource(const ResourceID& id) {
        // TODO: Locking.

        const auto it{ resourcesById_.find(id) };
        if (it != resourcesById_.end()) {
            UGINE_DEBUG("Deregister resources: {}", it->second.path.String());

            resourcesByPath_.erase(it->second.path);
            resourcesById_.erase(id);
        
            UGINE_ASSERT(resourcesById_.size() == resourcesByPath_.size());
        }
    }

    const std::unordered_map<ResourceID, ResourceRef>& RegisteredResources() const {
        // TODO: Locking.
        return resourcesById_;
    }

    Path ResourcePath(const ResourceID& id) const {
        // TODO: Locking.
        const auto it{ resourcesById_.find(id) };
        return it != resourcesById_.end() ? it->second.path : Path{};
    }

    const String& ResourceName(const ResourceID& id) const {
        // TODO: Locking.
        static String null{ "<no resource>" };

        const auto it{ resourcesById_.find(id) };
        return it != resourcesById_.end() ? it->second.name : null;
    }

    ResourceType ResourceType(const ResourceID& id) const {
        // TODO: Locking.
        const auto it{ resourcesById_.find(id) };
        return it != resourcesById_.end() ? it->second.type : ugine::ResourceType{};
    }

    ResourceID FindResource(const Path& path) {
        const auto it{ resourcesByPath_.find(path) };
        return it != resourcesByPath_.end() ? it->second : ResourceID{};
    }

    void SetResourcePath(const ResourceID& id, const Path& path) {
        auto it{ resourcesById_.find(id) };
        UGINE_ASSERT(it != resourcesById_.end());
        if (it == resourcesById_.end()) {
            return;
        }

        const auto origPath{ ResourcePath(id) };

        UGINE_ASSERT(resourcesByPath_.contains(origPath));
        UGINE_ASSERT(!resourcesByPath_.contains(path));

        it->second.path = path;

        resourcesByPath_.erase(origPath);
        resourcesByPath_.insert(std::make_pair(path, id));

        UGINE_ASSERT(resourcesByPath_.size() == resourcesById_.size());
    }

private:
    template <typename T> ResourceStorageBase* GetStorage(bool createNonExistent = true) {
        auto storage{ storages_.find(T::TYPE) };
        if (storage == storages_.end()) {
            if (!createNonExistent) {
                return nullptr;
            }

            storage = storages_.insert(std::make_pair(T::TYPE, MakeUnique<ResourceStorage<T>>(allocator_, allocator_))).first;
        }

        return storage->second.Get();
    }

    ResourceStorageBase* GetStorage(const ugine::ResourceType& type) const {
        auto storage{ storages_.find(type) };
        return storage == storages_.end() ? nullptr : storage->second.Get();
    }

    Engine& engine_;
    IAllocator& allocator_;

    std::unordered_map<ResourceID, ResourceRef> resourcesById_;
    std::unordered_map<Path, ResourceID> resourcesByPath_;

    std::unordered_map<ugine::ResourceType, UniquePtr<ResourceStorageBase>> storages_;

    inline static std::unordered_map<ResourceID, u64> NullMap;
};

} // namespace ugine