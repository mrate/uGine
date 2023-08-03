#pragma once

#include <ugine/engine/core/Resource.h>

#include <ugine/SlotMap.h>

#include <unordered_map>

namespace ugine {

template <typename T> class ResourceStorage final : public ResourceStorageBase {
public:
    using Storage = SlotMap<T>;
    using Key = Storage::KeyType;
    using Handle = ResourceHandle<T>;

    explicit ResourceStorage(IAllocator& allocator)
        : storage_{ allocator } {}

    ~ResourceStorage() {
        //UGINE_ASSERT(storage_.Empty());
    }

    ResourceHandleTypeless Create(ResourceManager& manager, const ResourceID& id) override {
        const auto handle{ storage_.Emplace(manager, id) };
        cache_.insert(std::make_pair(id, handle));
        return ResourceHandleTypeless{ storage_.Get(handle) };
    }

    ResourceHandleTypeless Get(const ResourceID& id) override {
        const auto it{ cache_.find(id) };
        return it != cache_.end() ? ResourceHandleTypeless{ storage_.Get(it->second) } : ResourceHandleTypeless{};
    }

    void Delete(const ResourceID& id) override {
        const auto it{ cache_.find(id) };
        if (it != cache_.end()) {
            storage_.Erase(it->second);
            cache_.erase(id);
        }
    }
    void Clear() override {
        storage_.Clear();
        cache_.clear();
    }

    const std::unordered_map<ResourceID, u64>& All() { return cache_; }

protected:
    std::unordered_map<ResourceID, u64>::iterator Add(T&& resource) {
        const auto id{ resource.Id() };
        const auto key{ storage_.Emplace(std::move(resource)) };

        return cache_.insert(std::make_pair(id, key)).first;
    }

    Storage storage_;
    std::unordered_map<ResourceID, u64> cache_;
};

} // namespace ugine