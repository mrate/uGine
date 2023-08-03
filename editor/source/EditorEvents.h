#pragma once

#include <ugine/Path.h>
#include <ugine/Span.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/world/World.h>

namespace ugine {
class World;
}

namespace ugine::ed {

class ModalWindow;

struct LogEvent {
    LogLevel level;
    u64 millis;
    Span<const char> msg;
};

struct ResourceReference {
    ResourceID id;
    ResourceType type;
};

using ResourceReferences = Vector<ResourceReference>;

template <typename T> struct ResourceCreatedEvent {
    ResourceHandle<T> resource;
};

struct ResourceDeletedEvent {
    ResourceReference resource;
};

struct ResourceIdContextMenuEvent {
    ResourceID id;
};

template <typename T> struct ResourceContextMenuEvent {
    ResourceHandle<T> resource;
};

struct ErrorEvent {
    String error;
};

struct ShowModalWindowEvent {
    ModalWindow* window{};
};

struct CloseModalWindowEvent {
    ModalWindow* window{};
};

template <typename T> struct ImportResourceEvent {
    Path path;
};

template <typename T> struct OpenResourceEvent {
    ResourceHandle<T> resource;
};

struct ReloadResourceEvent {
    ResourceReference resource{};
};

struct OpenTypelessResourceEvent {
    ResourceReference resource{};
};

struct OpenProjectEvent {
    Path path;
};

struct OpenWorldEvent {
    ResourceHandle<WorldDescriptor> world;
    bool clear{};
};

struct SaveWorldEvent {};

struct WorldChangedEvent {
    World* world{};
};

struct GameObjectSelectedEvent {
    GameObject gameObject;
};

struct AssetModifiedEvent {
    ResourceReference resource{};
    Path path;
};

struct RegisterAssetWatcherEvent {
    ResourceReference resource{};
    Path path;
};

struct DeregisterAssetWatcherEvent {
    ResourceReference resource{};
    Path path;
};

struct LocateAssetEvent {
    ResourceID id;
};

struct MoveAssetEvent {
    Path destination;
    ResourceID id;
};

struct AssetPathChagnedEvent {
    Path path;
};

struct StartWorldSimulationEvent {};
struct StopWorldSimulationEvent {};
struct PauseWorldSimulationEvent {
    bool pause{};
};

struct GameObjectDeselectedEvent {};

struct ResetThumbnailEvent {
    ResourceID id{};
};

class EditorEvents : public EventEmittor {
public:
    void Log(LogLevel level, u64 millis, Span<const char> message) { Emit(LogEvent{ level, millis, message }); }

    bool Trigger() { return Update(); }

    void Error(StringView error) { Enqueue(ErrorEvent{ error }); }

    void ShowModal(ModalWindow* window) { Enqueue(ShowModalWindowEvent{ window }); }
    void CloseModal(ModalWindow* window) { Enqueue(CloseModalWindowEvent{ window }); }

    void OpenProject(const Path& path) { Enqueue(OpenProjectEvent{ path }); }

    void OpenWorld(ResourceHandle<WorldDescriptor> world, bool clear = true) { Enqueue(OpenWorldEvent{ world, clear }); }
    void SaveWorld() { Enqueue(SaveWorldEvent{}); }

    void ChangeWorld(World* world) {
        UGINE_ASSERT(world);

        Emit(WorldChangedEvent{ world });
    }

    void LocateAsset(const ResourceID& id) { Enqueue(LocateAssetEvent{ id }); }

    void OpenTypelessResource(const ResourceReference& resource) { Enqueue(OpenTypelessResourceEvent{ resource }); }
    template <typename T> void ImportResource(const Path& path) { Enqueue(ImportResourceEvent<T>{ path }); }
    template <typename T> void OpenResource(ResourceHandle<T> handle) { Enqueue(OpenResourceEvent{ handle }); }
    template <typename T> void ResourceCreated(ResourceHandle<T> handle) { Enqueue(ResourceCreatedEvent<T>{ handle }); }
    void ResourceDeleted(const ResourceReference& resource) { Enqueue(ResourceDeletedEvent{ resource }); }
    void ReloadResource(const ResourceReference& resource) { Enqueue(ReloadResourceEvent{ resource }); }
    template <typename T> void ReloadResource(const ResourceHandle<T>& resource) {
        Enqueue(ReloadResourceEvent{ ResourceReference{
            .id = resource->Id(),
            .type = resource->Type(),
        } });
    }

    void AssetModified(const ResourceReference& resource, const Path& path) { Enqueue(AssetModifiedEvent{ resource, path }); }
    void RegisterAssetWatcher(const ResourceReference& resource, const Path& path) { Enqueue(RegisterAssetWatcherEvent{ resource, path }); }
    void DeregisterAssetWatcher(const ResourceReference& resource, const Path& path) { Enqueue(DeregisterAssetWatcherEvent{ resource, path }); }

    void MoveResource(const Path& path, const ResourceID& id) { Enqueue(MoveAssetEvent{ path, id }); }

    void AssetPathSelected(const Path& path) { Enqueue(AssetPathChagnedEvent{ path }); }

    void StartWorldSimulation() { Enqueue(StartWorldSimulationEvent{}); }
    void StopWorldSimulation() { Enqueue(StopWorldSimulationEvent{}); }
    void PauseWorldSimulation(bool pause) { Enqueue(PauseWorldSimulationEvent{ pause }); }

    void ResetThumbnail(const ResourceID& id) { Enqueue(ResetThumbnailEvent{ id }); }

    template <typename T, typename... Args> void CustomEvent(Args&&... args) { Enqueue(T(std::forward<Args>(args)...)); }
};

} // namespace ugine::ed