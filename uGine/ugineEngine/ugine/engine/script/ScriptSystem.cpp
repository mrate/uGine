#include "ScriptSystem.h"
#include "Component.h"
#include "ScriptScene.h"
#include "ScriptState.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/world/World.h>
#include <ugine/engine/world/WorldManager.h>

namespace ugine {

ScriptSystem::ScriptSystem(Engine& engine)
    : System{ engine } {
    state_ = &GetEngine().AttachState<ScriptState>(engine);

    engine.GetWorldManager().Connect<WorldManager::WorldCreatedEvent, &ScriptSystem::OnWorldCreated>(this);
}

ScriptSystem::~ScriptSystem() {
    GetEngine().GetResources().Clear<LuaScript>();

    state_ = nullptr;
    GetEngine().DeleteState<ScriptState>();
}

void ScriptSystem::Update() {
    auto& wm{ GetEngine().GetWorldManager() };

    wm.ForEachScene<ScriptScene>([&](ScriptScene& scene) {
        auto& world{ scene.GetWorld() };
        if (!world.IsSimulating()) {
            return;
        }

        scene.Update();
    });
}

void ScriptSystem::OnWorldCreated(const WorldManager::WorldCreatedEvent& event) {
    event.world->AddScene(MakeUnique<ScriptScene>(GetEngine().GetAllocator(), GetEngine(), *event.world, *state_, GetEngine().GetAllocator()));
}

} // namespace ugine