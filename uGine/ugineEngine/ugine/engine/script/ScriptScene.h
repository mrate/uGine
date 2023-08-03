#pragma once

#include <ugine/engine/world/GameObject.h>
#include <ugine/engine/world/World.h>

#include <ugine/String.h>

namespace ugine {

class World;
class ScriptState;
class GameObject;

struct LuaScriptRuntime;

class ScriptScene final : public WorldScene {
public:
    inline static constexpr StringID NAME{ "ScriptScene" };

    ScriptScene(Engine& engine, World& world, ScriptState& state, IAllocator& allocator = IAllocator::Default());
    ~ScriptScene();

    void Update() override;
    StringID Name() const { return NAME; }

private:
    void LuaScriptCreated(GameObjectRegistry& reg, GameObjectHandle ent);
    void LuaScriptDestroyed(GameObjectRegistry& reg, GameObjectHandle ent);
    void LuaScriptUpdated(GameObjectRegistry& reg, GameObjectHandle ent);

    void Attach(GameObject& go, LuaScriptRuntime& script, StringView code);
    void Detach(GameObject& go, LuaScriptRuntime& script);
    void Update(GameObject& go, LuaScriptRuntime& script);

    ScriptState& state_;
};

} // namespace ugine