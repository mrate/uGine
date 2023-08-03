#include "LuaScript.h"

namespace ugine {

bool LuaScript::HandleLoad(Span<const u8> data) {
    source = String{ Span<const char>{ reinterpret_cast<const char*>(data.Data()), data.Size() }, Manager().GetAllocator() };
    return true;
}

bool LuaScript::HandleUnload() {
    source.Clear();
    return true;
}

} // namespace ugine