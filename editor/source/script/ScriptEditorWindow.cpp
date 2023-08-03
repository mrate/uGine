#include "ScriptEditorWindow.h"
#include "../EditorContext.h"

#include "../widgets/ImGuiScope.h"

#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/script/ScriptState.h>

#include <ugine/Profile.h>

namespace ugine::ed {

ScriptEditorWindow::ScriptEditorWindow(EditorContext& context)
    : EditorTool{ context, false } {
    auto& viewMenu{ context_.MainMenu().Get("View") };
    viewMenu.AddAction([this] { return ImGui::Checkbox("Script editor", &visible_); });

    context_.Events().Connect<OpenTypelessResourceEvent, &ScriptEditorWindow::OnOpenTypelessResource>(this);
    context_.Events().Connect<OpenResourceEvent<LuaScript>, &ScriptEditorWindow::OnOpenScript>(this);

    document_.Resize(1024 * 1024);
}

void ScriptEditorWindow::SetScript(ResourceHandle<LuaScript> script) {
    script_ = script;
    loaded_ = script->Ready();
    visible_ = true;

    if (loaded_) {
        SyncScript();
    }
}

void ScriptEditorWindow::Render() {
    if (!script_) {
        return;
    }

    PROFILE_EVENT_NC("ScriptEditorWindow", COLOR_EDITOR_PROFILE);

    if (ImGui::Begin("Script editor", &visible_)) {
        if (loaded_) {
            if (ImGui::Button(ICON_FA_CONTENT_SAVE)) {
                //context_.RefreshResource(script_);
                // TODO: [resources]
                // , SerializedLuaScript{ document_.data() }
                script_->source = document_.Data();
            }

            ImGuiEx::ToolTipText("Save script");

            ImGui::InputTextMultiline("##script", document_.Data(), document_.Size(), ImGui::GetContentRegionAvail());

            if (!error_.Empty()) {
                ImScope::Color color{ ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 } };
                ImGui::TextUnformatted(error_.Data());
            }
        } else {
            if (script_->Ready()) {
                loaded_ = true;
                SyncScript();
            } else {
                ImGuiEx::Loading();
            }
        }
    }

    ImGui::End();
}

void ScriptEditorWindow::OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
    if (event.resource.type == LuaScript::TYPE) {
        context_.RunTextEditor(context_.Engine().GetResources().ResourcePath(event.resource.id).String());

        //SetScript(context_.Engine().GetResources().Get<LuaScript>(event.resource.id));
    }
}

void ScriptEditorWindow::SyncScript() {
    document_.Clear();
    if (script_->source.Empty()) {
        document_.Resize(1024 * 1024);
    } else {
        if (document_.Size() < 2 * script_->source.Size()) {
            document_.Resize(2 * script_->source.Size());
        }

        memcpy(document_.Data(), script_->source.Data(), script_->source.Size() + 1);
    }
}

void ScriptEditorWindow::OnOpenScript(const OpenResourceEvent<LuaScript>& event) {
    context_.RunTextEditor(context_.Engine().GetResources().ResourcePath(event.resource->Id()).String());

    //SetScript(event.resource);
}

} // namespace ugine::ed