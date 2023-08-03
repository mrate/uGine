#include "ScriptState.h"

#include <ugine/Log.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/script/Lua.h>
#include <ugine/engine/world/GameObject.h>
#include <ugine/engine/world/World.h>
#include <ugine/engine/world/WorldManager.h>

#include <format>

using namespace luabridge;

namespace ugine {

void Bind(lua_State* state, Engine& engine) {
    try {
        RegisterLuaState(state);
    } catch (const std::exception& ex) {
        UGINE_ERROR(ex.what());
    }

    getGlobalNamespace(state).beginNamespace("ugine").addProperty("engine", [&] { return &engine; }).endNamespace();
}

ScriptState::ScriptState(Engine& engine)
    : allocator_{ engine.GetAllocator() } {
    luaState_ = luaL_newstate();
    luaL_openlibs(luaState_);

    Bind(luaState_, engine);
}

ScriptState::~ScriptState() {
    lua_close(luaState_);
}

ScriptState::Result ScriptState::Execute(Span<const char> code) {
    const auto error{ luaL_dostring(luaState_, code.Data()) };

    String output{ allocator_ };
    if (error) {
        output = lua_tostring(luaState_, -1);

        UGINE_ERROR("Lua error: {}", output.Data());
    }

    return Result{
        .success = error == 0,
        .error = output,
    };
}

std::string ScriptState::GenNameSpace() {
    return std::format("script_{}", namespaceCounter_++);
}

} // namespace ugine