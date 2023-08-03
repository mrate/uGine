#include "WorldManager.h"

#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/script/Component.h>

#include <algorithm>

namespace ugine {

class Cloner {
public:
    entt::registry& target_;
    std::map<entt::entity, entt::entity> srcToDst_;

    explicit Cloner(entt::registry& target)
        : target_{ target } {}

    void operator()(std::underlying_type_t<entt::entity> size) {}
    void operator()(entt::entity entity) {
        const auto newEnt{ target_.create(entity) };
        srcToDst_[entity] = newEnt;
    }

    template <typename T> void operator()(entt::entity ent, const T& t) {
        const auto target{ srcToDst_.at(ent) };
        target_.emplace<T>(target, t);
    }

    template <> void operator()(entt::entity ent, const RelationshipComponent& t) {
        const auto target{ srcToDst_.at(ent) };

        const RelationshipComponent newRel{
            .children = t.children,
            .firstChild = t.firstChild == entt::null ? entt::null : srcToDst_.at(t.firstChild),
            .lastChild = t.lastChild == entt::null ? entt::null : srcToDst_.at(t.lastChild),
            .prevSibling = t.prevSibling == entt::null ? entt::null : srcToDst_.at(t.prevSibling),
            .nextSibling = t.nextSibling == entt::null ? entt::null : srcToDst_.at(t.nextSibling),
        };

        target_.emplace<RelationshipComponent>(target, newRel);
    }

    template <> void operator()(entt::entity ent, const ParentComponent& t) {
        const auto target{ srcToDst_.at(ent) };
        const ParentComponent newParent{
            .parent = t.parent == entt::null ? entt::null : srcToDst_.at(t.parent),
        };
        target_.emplace<ParentComponent>(target, newParent);
    }
};

WorldManager::WorldManager(Engine& engine)
    : engine_{ engine }
    , worlds_{ engine.GetAllocator() } {
}

WorldManager::~WorldManager() {
}

void WorldManager::DestroyWorlds() {
    worlds_.Clear();
}

World* WorldManager::CreateWorld(u32 userFlags) {
    UGINE_DEBUG("World created");

    worlds_.PushBack(World::Create(engine_.GetAllocator(), userFlags));
    auto world{ worlds_.Back().Get() };

    Emit(WorldCreatedEvent{ .world = world });

    return world;
}

void WorldManager::DestroyWorld(World* world) {
    toDestroy_.PushBack(world);
}

World* WorldManager::Clone(World& source) {
    auto newWorld{ CreateWorld() };

    // TODO:
    auto& src{ source.registry_ };
    auto& dst{ newWorld->registry_ };

    Cloner cloner{ dst };
    entt::basic_snapshot snapshot{ src };
    snapshot.entities(cloner)
        .component<                       //
            TagComponent,                 //
            StaticFlagComponent,          //
            RelationshipComponent,        //
            ParentComponent,              //
            TransformationComponent,      //
            CameraComponent,              //
            MeshComponent,                //
            AnimationControllerComponent, //
            LightComponent,               //
            LuaScriptComponent            //
            >(cloner);

    return newWorld;
}

void WorldManager::SyncPoint() {
    for (auto world : toDestroy_) {
        if (world == defaultWorld_) {
            defaultWorld_ = nullptr;
        }

        const auto index{ worlds_.FindIf([world](auto& w) { return w.Get() == world; }) };
        if (index >= 0) {
            Emit(WorldDestroyedEvent{
                .world = worlds_[index].Get(),
            });

            UGINE_DEBUG("World destroyed");

            worlds_.EraseAt(index);
        }
    }

    toDestroy_.Clear();
}

} // namespace ugine