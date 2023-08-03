#pragma once

#include <lua.hpp>

#include <LuaBridge/LuaBridge.h>

namespace ugine {

void RegisterLuaState(lua_State* state);

} // namespace ugine