#pragma once
#pragma once

#include "World.h"

#include <memory>
#include <stdint.h>
#include <vector>

#include <ugine/EventEmittor.h>
#include <ugine/engine/script/Script.h>

namespace ugine {

class WorldManager final : public EventEmittor {
public:
    struct WorldCreatedEvent {
        World* world{};
    };

    struct WorldDestroyedEvent {
        World* world{};
    };

    explicit WorldManager(Engine& engine);
    ~WorldManager();

    void DestroyWorlds();

    Engine& GetEngine() { return engine_; }

    void SetDefaultWorld(World* world) { defaultWorld_ = world; }
    World* GetDefaultWorld() { return defaultWorld_; }

    World* CreateWorld(u32 userFlags = 0);
    void DestroyWorld(World* world);

    World* Clone(World& source);

    const Vector<UniquePtr<World>>& Worlds() const { return worlds_; }
    u32 Size() const { return u32(worlds_.Size()); }
    World* GetWorld(u32 index) { return index < worlds_.Size() ? worlds_[index].Get() : nullptr; }

    template <typename T, typename F> void ForEachScene(F f) {
        for (auto& world : worlds_) {
            if (auto scene = world->GetScene(T::NAME)) {
                f(*static_cast<T*>(scene));
            }
        }
    }

    void SyncPoint();

private:
    Engine& engine_;
    Vector<UniquePtr<World>> worlds_;
    Vector<World*> toDestroy_;
    World* defaultWorld_{};
};

} // namespace ugine