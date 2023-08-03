#include "ScriptingWindow.h"
#include "../EditorContext.h"
#include "ScriptTool.h"

#include "../widgets/ImGuiScope.h"

#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/script/ScriptState.h>

#include <ugine/Profile.h>

namespace ugine::ed {

constexpr auto SETTINGS_MACROS{ "script-macros" };

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScriptMacro, name, code);

ScriptingWindow::ScriptingWindow(EditorContext& context)
    : context_{ context } {

    auto& debugMenu{ context_.MainMenu().Get("Debug") };
    debugMenu.AddAction([this] { return ImGui::Checkbox("Scripting", &visible_); });

    scriptName_.Resize(1024);
    scriptCode_.Resize(1024 * 1024);

    SetRightButtons({
        Button{
            .name = ICON_FA_ARROW_RIGHT " Run [F5]",
            .func = [this] { Execute(); },
        },
    });

    macros_ = context_.Settings().Get<Vector<ScriptMacro>>(SETTINGS_MACROS);
}

void ScriptingWindow::Render() {
    if (!visible_) {
        return;
    }

    PROFILE_EVENT_NC("ScriptingWindow", COLOR_EDITOR_PROFILE);

    if (ImGui::Begin(ICON_FA_CLIPBOARD_LIST " Scripting", &visible_)) {
        BeginContent();

        //const auto size{ ImGui::GetContentRegionAvail() };
        if (ImGui::BeginTable("##scripting", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            MacroList();

            ImGui::TableNextColumn();

            MacroContent();

            ImGui::EndTable();
        }

        EndContent();

        Buttons();

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F5)) {
            Execute();
        }
    }

    ImGui::End();
}

void ScriptingWindow::Execute() {
    error_.Clear();
    ExecuteScript(context_, scriptCode_.ToSpan(), error_);
}

void ScriptingWindow::AddMacro() {
    macros_.PushBack(ScriptMacro{
        .name = scriptName_.Data(),
        .code = scriptCode_.Data(),
    });

    SaveMacros();
}

void ScriptingWindow::RemoveMacro() {
    if (selectedMacro_ >= 0 && selectedMacro_ < macros_.Size()) {
        macros_.EraseAt(selectedMacro_);
        selectedMacro_ = -1;

        SaveMacros();
    }
}

void ScriptingWindow::SaveMacro(int index) {
    if (index >= 0 && index < macros_.Size()) {
        macros_[index].name = scriptName_.Data();
        macros_[index].code = scriptCode_.Data();

        SaveMacros();
    } else {
        AddMacro();
    }
}

void ScriptingWindow::SaveMacros() {
    context_.Settings().Set(SETTINGS_MACROS, macros_);
    context_.SaveSettings();

    context_.Events().CustomEvent<ScriptMacrosChangedEvent>();
}

void ScriptingWindow::MacroList() {
    ImScope::ItemWidth width{ -1 };

    const auto size{ ImGui::GetContentRegionAvail() };
    const auto padding{ ImGui::GetStyle().FramePadding };
    const auto spacing{ ImGui::GetStyle().ItemSpacing };

    if (ImGui::BeginListBox("##macrolist", ImVec2{ size.x, size.y - ImGui::GetFontSize() - 2 * padding.y - spacing.y })) {
        for (int index{}; const auto& [name, _] : macros_) {
            ImScope::Id id{ index };

            bool selected{ selectedMacro_ == index };
            if (ImGui::Selectable(name.Data(), &selected)) {
                SelectMacro(index);
            }

            ++index;
        }

        ImGui::EndListBox();
    }

    {
        ImScope::Disabled disabled{ scriptName_[0] == '\0' };

        if (ImGui::Button(ICON_FA_PLUS)) {
            AddMacro();
        }

        ImGuiEx::ToolTipText("Add macro");
    }

    ImGui::SameLine();

    {
        ImScope::Disabled diabled{ selectedMacro_ < 0 || selectedMacro_ >= macros_.Size() };

        if (ImGui::Button(ICON_FA_MINUS)) {
            RemoveMacro();
        }

        ImGuiEx::ToolTipText("Remove macro");
    }
}

void ScriptingWindow::MacroContent() {
    ImGui::BeginGroup();
    ImGui::InputText("##documentName", scriptName_.Data(), scriptName_.Size());
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FLOPPY)) {
        SaveMacro(selectedMacro_);
    }
    ImGuiEx::ToolTipText("Save macro");

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE)) {
        SelectMacro(-1);
    }
    ImGuiEx::ToolTipText("New macro");

    ImGui::EndGroup();

    if (!error_.Empty()) {
        ImScope::Color color{ ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 } };
        ImGui::TextUnformatted(error_.Data());
    }

    const auto size{ ImGui::GetContentRegionAvail() };
    ImGui::InputTextMultiline("##script", scriptCode_.Data(), scriptCode_.Size(), size);
}

void ScriptingWindow::SelectMacro(int index) {
    if (index >= 0 && index < macros_.Size()) {
        selectedMacro_ = index;
        const auto& macro{ macros_[selectedMacro_] };

        memcpy(scriptCode_.Data(), macro.code.Data(), macro.code.Size());
        scriptCode_[macro.code.Size()] = '\0';

        memcpy(scriptName_.Data(), macro.name.Data(), macro.name.Size());
        scriptName_[macro.name.Size()] = '\0';
    } else {
        selectedMacro_ = -1;

        scriptCode_[0] = '\0';
        scriptName_[0] = '\0';
    }
}

} // namespace ugine::ed