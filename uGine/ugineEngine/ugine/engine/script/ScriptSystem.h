#pragma once

#include <ugine/engine/engine/System.h>
#include <ugine/engine/world/WorldManager.h>

namespace ugine {

class ScriptState;

class ScriptSystem final : public System {
public:
    ScriptSystem(Engine& engine);
    ~ScriptSystem();

    void Update() override;

private:
    void OnWorldCreated(const WorldManager::WorldCreatedEvent& event);

    ScriptState* state_{};
};

} // namespace ugine