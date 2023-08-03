#pragma once

#include <ugine/EventEmittor.h>
#include <ugine/FileSystem.h>
#include <ugine/Hash.h>
#include <ugine/SlotMap.h>

#include <ugine/engine/core/ResourceID.h>

#include <array>
#include <unordered_map>

namespace ugine {

class ResourceManager;

class ResourceType {
public:
    ResourceType() = default;
    explicit ResourceType(StringView type)
        : id_{ type }
        , name_{ type } {}

    const char* Name() const { return name_.Data(); }
    StringID Hash() const { return id_; }

private:
    StringID id_{};
    String name_{};
};

UGINE_FORCE_INLINE bool operator==(const ResourceType& a, const ResourceType& b) {
    return a.Hash() == b.Hash();
}

UGINE_FORCE_INLINE bool operator!=(const ResourceType& a, const ResourceType& b) {
    return !(a == b);
}

UGINE_FORCE_INLINE bool operator<(const ResourceType& a, const ResourceType& b) {
    return a.Hash() < b.Hash();
}

enum class ResourceState {
    Unloaded,
    Loading,
    Loaded,
    Failed,
};

inline static constexpr const char* ResourceStateName[] = {
    "Unloaded",
    "Loading",
    "Loaded",
    "Failed",
};

class Resource : public EventEmittorMT {
public:
    struct StateChangedEvent {
        ResourceID id{};
        Resource* resource{};
        ResourceState newState{};
        ResourceState prevState{};
    };

    Resource(ResourceManager& resourceManager, const ResourceType& type, const ResourceID& id)
        : resourceManager_{ resourceManager }
        , type_{ type }
        , id_{ id } {}

    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource(Resource&&) = default;
    Resource& operator=(Resource&&) = default;

    virtual ~Resource();

    u64 IncRef();
    u64 DecRef();
    u64 References() const { return refCnt_; }

    const ResourceID& Id() const { return id_; }
    ResourceState State() const { return state_; }
    const ResourceType& Type() const { return type_; }

    bool Ready() const { return state_ == ResourceState::Loaded; }

    void LoadAsync(StringView file);
    void Load(Span<const u8> data);
    void Unload();

protected:
    virtual bool HandleLoad(Span<const u8> data) { return false; }
    virtual bool HandleUnload() { return true; }
    virtual void HandleDependenciesReady() {}
    virtual void HandleDependencyChanged(const ResourceID& id, ResourceState state) {}

    void SetState(ResourceState newState);

    void AddDependency(Resource* resource);
    void RemoveDependency(Resource* resource);
    u32 LoadingDependencies() const { return loadingDependencies_; }

    ResourceManager& Manager() { return resourceManager_; }
    const ResourceManager& Manager() const { return resourceManager_; }

private:
    void OnDependencyChanged(const StateChangedEvent& event);

    ResourceManager& resourceManager_;
    ResourceType type_;
    ResourceID id_;
    ResourceState state_{ ResourceState::Unloaded };

    // TODO: Atomic
    u32 refCnt_{};

    //std::atomic_uint32_t refCnt_{};

    u32 loadingDependencies_{};
    u32 dependencies_{};

    FileSystem::RequestHandle ioRequest_{};
};

class ResourceHandleTypeless {
public:
    ResourceHandleTypeless() = default;

    explicit ResourceHandleTypeless(Resource* resource)
        : resource_{ resource } {
        UGINE_ASSERT(resource);
        resource_->IncRef();
    }

    ~ResourceHandleTypeless() {
        if (resource_) {
            resource_->DecRef();
            resource_ = nullptr;
        }
    }

    ResourceHandleTypeless(const ResourceHandleTypeless& other)
        : resource_{ other.resource_ } {
        if (resource_) {
            resource_->IncRef();
        }
    }

    ResourceHandleTypeless& operator=(const ResourceHandleTypeless& other) {
        if (resource_) {
            resource_->DecRef();
        }
        resource_ = other.resource_;
        if (resource_) {
            resource_->IncRef();
        }
        return *this;
    }

    ResourceHandleTypeless(ResourceHandleTypeless&& other) noexcept
        : resource_{ other.resource_ } {
        other.resource_ = nullptr;
    }

    ResourceHandleTypeless& operator=(ResourceHandleTypeless&& other) noexcept {
        if (resource_) {
            resource_->DecRef();
        }
        resource_ = other.resource_;
        other.resource_ = nullptr;
        return *this;
    }

    explicit operator bool() const { return resource_ != nullptr; }

    Resource* Get() { return resource_; }
    Resource* Get() const { return resource_; }

    Resource* operator->() { return Get(); }
    Resource* operator->() const { return Get(); }

protected:
    Resource* resource_{};
};

template <typename T> class ResourceHandle : public ResourceHandleTypeless {
public:
    ResourceHandle() = default;

    explicit ResourceHandle(Resource* resource) noexcept
        : ResourceHandleTypeless{ resource } {}

    ResourceHandle(const ResourceHandle& other)
        : ResourceHandleTypeless{ other } {}

    ResourceHandle(const ResourceHandleTypeless& other)
        : ResourceHandleTypeless{ other } {
        UGINE_ASSERT(T::TYPE == other.Get()->Type());
    }

    T* operator*() { return Get(); }
    T* operator->() { return Get(); }
    T* Get() { return static_cast<T*>(resource_); }

    T* operator*() const { return Get(); }
    T* operator->() const { return Get(); }
    T* Get() const { return static_cast<T*>(resource_); }
};

template <typename T> UGINE_FORCE_INLINE bool operator==(const ResourceHandle<T>& a, const ResourceHandle<T>& b) {
    return a.Get() == b.Get();
}

template <typename T> UGINE_FORCE_INLINE bool operator!=(const ResourceHandle<T>& a, const ResourceHandle<T>& b) {
    return !(a == b);
}

template <typename T> UGINE_FORCE_INLINE bool operator==(const ResourceHandleTypeless& a, const ResourceHandle<T>& b) {
    return a.Get() == b.Get();
}

template <typename T> UGINE_FORCE_INLINE bool operator==(const ResourceHandle<T>& a, const ResourceHandleTypeless& b) {
    return a.Get() == b.Get();
}

UGINE_FORCE_INLINE bool operator==(const ResourceHandleTypeless& a, const ResourceHandleTypeless& b) {
    return a.Get() == b.Get();
}

template <typename T> UGINE_FORCE_INLINE bool operator!=(const ResourceHandleTypeless& a, const ResourceHandle<T>& b) {
    return !(a == b);
}

template <typename T> UGINE_FORCE_INLINE bool operator!=(const ResourceHandle<T>& a, const ResourceHandleTypeless& b) {
    return !(a == b);
}

UGINE_FORCE_INLINE bool operator!=(const ResourceHandleTypeless& a, const ResourceHandleTypeless& b) {
    return !(a == b);
}

class ResourceStorageBase {
public:
    virtual ~ResourceStorageBase() = default;

    virtual ResourceHandleTypeless Create(ResourceManager& manager, const ResourceID& id) = 0;
    virtual ResourceHandleTypeless Get(const ResourceID& id) = 0;
    virtual void Delete(const ResourceID& id) = 0;
    virtual void Clear() = 0;
    virtual const std::unordered_map<ResourceID, u64>& All() = 0;
};

} // namespace ugine

// TODO: Move somewhere.
namespace std {

template <> struct hash<ugine::ResourceID> {
    size_t operator()(const ugine::ResourceID& v) const noexcept {
        size_t res{ 0 };
        ugine::HashCombine(res, v.uuid);
        return res;
    }
};

template <> struct hash<ugine::ResourceType> {
    size_t operator()(const ugine::ResourceType& v) const noexcept { return v.Hash(); }
};

template <> struct hash<ugine::ResourceHandleTypeless> {
    size_t operator()(const ugine::ResourceHandleTypeless& v) const noexcept { return v.Get() ? hash<ugine::UUID>{}(v->Id().uuid) : 0; }
};

} // namespace std

namespace ugine {

class ResourceListener {
public:
    template <auto Handler, typename... Args> void Connect(Resource& resource, Args&&... args) {
        auto& count{ connections_[resource.Id()] };
        if (count++ == 0) {
            resource.Connect<Resource::StateChangedEvent, Handler>(std::forward<Args>(args)...);
        }
    }

    template <typename... Args> void Disconnect(Resource& resource, Args&&... args) {
        auto& count{ connections_[resource.Id()] };
        UGINE_ASSERT(count > 0);
        if (--count == 0) {
            connections_.erase(resource.Id());
            resource.Disconnect<Resource::StateChangedEvent>(std::forward<Args>(args)...);
        }
    }

private:
    std::unordered_map<ResourceID, u32> connections_;
};

} // namespace ugine