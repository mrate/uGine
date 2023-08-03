#include "ScriptTool.h"

#include "../EditorContext.h"
#include "../EditorTool.h"

#include "ScriptEditorWindow.h"
#include "ScriptingWindow.h"

#include <ugine/Memory.h>

#include <ugine/engine/script/ScriptState.h>

namespace ugine::ed {

bool ExecuteScript(EditorContext& context, Span<const char> script, String& error) {
    if (auto state{ context.Engine().GetState<ScriptState>() }; state) {
        const auto result{ state->Execute(script) };
        if (result.success) {
            return true;
        }

        error = result.error;
    } else {
        error = "Scripting not available.";
    }

    return false;
}

class ScriptTool : public EditorTool {
public:
    explicit ScriptTool(EditorContext& context)
        : EditorTool{ context }
        , scriptingWindow_{ context } {
        auto& fileMenu{ context_.MainMenu().Get("File") };
        fileMenu.AddAction(SCRIPT_RESOURCE_ICON " New script", [this] { CreateNewScript(); });
        fileMenu.AddSeparator();

        OnScriptMacrosChanged();
        context_.Events().Connect<ScriptMacrosChangedEvent, &ScriptTool::OnScriptMacrosChanged>(this);
    }

    void OnScriptMacrosChanged() {
        auto& macroMenu{ context_.MainMenu().Get("Macro") };
        macroMenu.Clear();

        // TODO:
        //for (const auto&& [name, script] : context_.Settings)
        //macroMenu.AddAction(SCRIPT_MACRO_ICON " Run", [this] { RunMacro(); });
    }

    void Render() override { scriptingWindow_.Render(); }

private:
    inline static const std::string NewScriptTemplate = R"script(
local script = {}

function script.attach(entity)
    ugine.debug("Attached")
end

function script.detach(entity)
    ugine.debug("Detached")
end

function script.update(entity)
    ugine.debug("Update")
end

return script
)script";

    void CreateNewScript() {
        auto [_, path] = context_.CreateResource<LuaScript>(context_.SelectedAssetPath(), "new_script");

        const Span scriptData{ reinterpret_cast<const u8*>(NewScriptTemplate.c_str()), NewScriptTemplate.size() };
        WriteFileBinary(path, scriptData);
    }

    void RunMacro() {
        auto document{ context_.Settings().Get<std::string>("editor-macro") };
        if (document.empty()) {
            return;
        }

        String error{};

        Span<const char> macro{ document.data(), document.size() };
        if (!ExecuteScript(context_, macro, error)) {
            context_.Events().Error(error.Data());
        }
    }

    ScriptingWindow scriptingWindow_;
};

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<LuaScript>(SCRIPT_RESOURCE_ICON, ".lua", ColorRGB{ 0.63f, 0.75f, 0.79f });

        context.RegisterEditorTool(MakeUnique<ScriptTool>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<ScriptEditorWindow>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed