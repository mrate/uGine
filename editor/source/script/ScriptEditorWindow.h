#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include <ugine/engine/script/LuaScript.h>

namespace ugine::ed {

class EditorContext;

class ScriptEditorWindow : public EditorTool {
public:
    explicit ScriptEditorWindow(EditorContext& context);
    void SetScript(ResourceHandle<LuaScript> script);

    void Render() override;

private:
    void SyncScript();

    void OnOpenScript(const OpenResourceEvent<LuaScript>& event);
    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event);

    bool loaded_{};
    Vector<char> document_;
    String error_;

    ResourceHandle<LuaScript> script_;
};

} // namespace ugine::ed