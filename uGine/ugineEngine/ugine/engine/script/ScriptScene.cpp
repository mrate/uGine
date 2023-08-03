#include "ScriptScene.h"

#include <ugine/engine/script/Component.h>
#include <ugine/engine/script/ScriptState.h>
#include <ugine/engine/world/GameObject.h>
#include <ugine/engine/world/World.h>

#include <lua.hpp>

#include <LuaBridge/LuaBridge.h>

namespace ugine {

struct LuaScriptRuntime {
    ResourceHandle<LuaScript> script;

    std::string name;
    bool attached{};
    std::optional<luabridge::LuaRef> attach;
    std::optional<luabridge::LuaRef> detach;
    std::optional<luabridge::LuaRef> update;
};

struct PendingLuaScriptFlag {};

ScriptScene::ScriptScene(Engine& engine, World& world, ScriptState& state, IAllocator& allocator)
    : WorldScene{ engine, world }
    , state_{ state } {
    auto& r{ world_.Registry() };

    r.on_construct<LuaScriptComponent>().connect<&GameObjectRegistry::emplace<LuaScriptRuntime>>();
    r.on_destroy<LuaScriptComponent>().connect<&SafeRemoveComponent<LuaScriptRuntime>>();
    r.on_update<LuaScriptComponent>().connect<&ScriptScene::LuaScriptUpdated>(this);
    r.on_construct<LuaScriptRuntime>().connect<&ScriptScene::LuaScriptCreated>(this);
    r.on_destroy<LuaScriptRuntime>().connect<&ScriptScene::LuaScriptDestroyed>(this);

    r.on_destroy<LuaScriptRuntime>().connect<&SafeRemoveComponent<PendingLuaScriptFlag>>();
}

ScriptScene::~ScriptScene() {
    auto& r{ world_.Registry() };

    r.clear<LuaScriptRuntime>();
    r.clear<PendingLuaScriptFlag>();

    r.on_construct<LuaScriptComponent>().disconnect(this);
    r.on_destroy<LuaScriptComponent>().disconnect(this);

    r.on_construct<LuaScriptRuntime>().disconnect();
    r.on_destroy<LuaScriptRuntime>().disconnect();
}

void ScriptScene::Update() {
    // TODO: Native script.

    for (auto&& [ent, runtime] : world_.Registry().view<LuaScriptRuntime, PendingLuaScriptFlag>().each()) {
        auto go{ world_.Get(ent) };
        if (runtime.script) {
            if (runtime.script->Ready()) {
                go.RemoveComponent<PendingLuaScriptFlag>();

                Attach(go, runtime, runtime.script->source.ToSpan());
            }
        } else {
            go.RemoveComponent<PendingLuaScriptFlag>();
        }
    }

    for (auto&& [ent, script] : world_.Registry().view<LuaScriptRuntime>().each()) {
        auto go{ world_.Get(ent) };
        if (go.IsEnabled()) {
            Update(go, script);
        }
    }
}

void ScriptScene::LuaScriptCreated(GameObjectRegistry& reg, GameObjectHandle ent) {
    const auto& comp{ reg.get<LuaScriptComponent>(ent) };

    if (!comp.script) {
        return;
    }

    auto& runtime{ reg.get<LuaScriptRuntime>(ent) };
    auto go{ world_.Get(ent) };

    runtime.name = state_.GenNameSpace();
    runtime.script = comp.script;

    // Always treat script as pending so OnAttached / OnDettached is called during update.
    if (runtime.script) {
        go.CreateComponent<PendingLuaScriptFlag>();
    }
}

void ScriptScene::LuaScriptDestroyed(GameObjectRegistry& reg, GameObjectHandle ent) {
    auto& runtime{ reg.get<LuaScriptRuntime>(ent) };
    auto go{ world_.Get(ent) };

    // TODO:
    if (runtime.attached) {
        Detach(go, runtime);
    }
}

void ScriptScene::LuaScriptUpdated(GameObjectRegistry& reg, GameObjectHandle ent) {
    auto& comp{ reg.get<LuaScriptComponent>(ent) };
    auto& runtime{ reg.get<LuaScriptRuntime>(ent) };
    auto go{ world_.Get(ent) };

    if (comp.script == runtime.script) {
        return;
    }

    // TODO:
    if (runtime.attached) {
        Detach(go, runtime);
    }

    runtime.script = comp.script;
    if (!go.Has<PendingLuaScriptFlag>()) {
        go.CreateComponent<PendingLuaScriptFlag>();
    }
}

void ScriptScene::Attach(GameObject& go, LuaScriptRuntime& script, StringView code) {
    using namespace luabridge;

    if (code.Empty()) {
        return;
    }

    auto load{ getGlobal(state_.LuaState(), "load") };

    auto loadedRes{ load(code.Data()) };
    if (!loadedRes || loadedRes.size() > 1) {
        const auto error{ loadedRes.size() > 1 ? loadedRes[1].tostring() : loadedRes.errorMessage() };
        UGINE_ERROR("Failed to attach script: {}", error);
        return;
    }

    auto loaded{ loadedRes[0] };

    auto executeRes{ loaded() };
    if (!executeRes || executeRes.size() > 1) {
        const auto error{ executeRes.size() > 1 ? executeRes[1].tostring() : executeRes.errorMessage() };
        UGINE_ERROR("Failed to attach script: {}", error);
        return;
    }

    auto executed{ executeRes[0] };
    setGlobal(state_.LuaState(), executed, script.name.c_str());
    script.attached = true;

    script.attach = getGlobal(state_.LuaState(), script.name.c_str())["attach"];
    script.detach = getGlobal(state_.LuaState(), script.name.c_str())["detach"];
    script.update = getGlobal(state_.LuaState(), script.name.c_str())["update"];

    // TODO: Parse params.
    auto params{ getGlobal(state_.LuaState(), script.name.c_str())["params"] };
    if (params.isTable()) {
        for (auto&& [key, value] : pairs(params)) {
            if (value.type() == LUA_TUSERDATA) {
                auto res{ value.cast<glm::vec3>() };
                if (res) {
                    UGINE_DEBUG("We have vector: {}, {}, {}", res.value().x, res.value().y, res.value().z);
                }
            }

            const auto typeName{ lua_typename(state_.LuaState(), value.type()) };

            UGINE_DEBUG("Param: '{}' = '{}' [{}]", key.tostring(), value.tostring(), typeName);
        }
    }

    if (script.attach && *script.attach) {
        auto attachRes{ (*script.attach)(&go) };
        if (!attachRes || attachRes.size() > 1) {
            const auto error{ attachRes.size() > 1 ? attachRes[1].tostring() : attachRes.errorMessage() };
            UGINE_ERROR("Failed to invoke attach(): {}", error);
        }
    }
}

void ScriptScene::Detach(GameObject& go, LuaScriptRuntime& script) {
    using namespace luabridge;

    if (!script.attached) {
        return;
    }

    try {
        // TODO: Disable detaching for now
        //if (script.detach && *script.detach) {
        //    (*script.detach)(&go);
        //}
    } catch (const std::exception& ex) {
        UGINE_ERROR("Script error: {}", ex.what());
    }

    setGlobal(state_.LuaState(), 0, script.name.c_str());

    script.attach = std::nullopt;
    script.detach = std::nullopt;
    script.update = std::nullopt;

    script.attached = false;
}

void ScriptScene::Update(GameObject& go, LuaScriptRuntime& script) {
    try {
        if (script.attached && script.update && *script.update) {
            auto res{ (*script.update)(&go) };
            if (!res || res.size() > 0) {
                const auto error{ res.size() > 0 ? res[0].tostring() : res.errorMessage() };
                UGINE_ERROR("Update error: {}", error);
            }
        }
    } catch (const std::exception& ex) {
        UGINE_ERROR("Script error: {}", ex.what());
    }
}

} // namespace ugine