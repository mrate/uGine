#pragma once

#include <ugine/Span.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>

#include <string>

struct lua_State;

namespace ugine {

class Engine;

class ScriptState {
public:
    struct Result {
        bool success{};
        String error{};
    };

    explicit ScriptState(Engine& engine);
    ~ScriptState();

    Result Execute(Span<const char> code);
    lua_State* LuaState() { return luaState_; }
    std::string GenNameSpace();

private:
    AllocatorRef allocator_;
    lua_State* luaState_{};
    u64 namespaceCounter_{};
};

} // namespace ugine