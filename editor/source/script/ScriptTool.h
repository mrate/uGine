#pragma once

#include <ugine/Span.h>
#include <ugine/String.h>

#define SCRIPT_RESOURCE_ICON ICON_FA_CLIPBOARD
#define SCRIPT_MACRO_ICON ICON_FA_HAMMER

namespace ugine::ed {

class EditorContext;

struct ScriptMacro {
    String name;
    String code;
};

struct ScriptMacrosChangedEvent {};

bool ExecuteScript(EditorContext& context, Span<const char> script, String& error);

} // namespace ugine::ed