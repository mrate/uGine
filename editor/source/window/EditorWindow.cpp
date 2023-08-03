#include "EditorWindow.h"
#include "../EditorContext.h"

#include <ugine/engine/gfx/ImGui.h>

namespace ugine::ed {

EditorWindow::EditorWindow(EditorContext& context, bool visible, StringView name, StringView id, int flags)
    : EditorTool{ context, visible }
    , name_{ name.Data() }
    , saveDialog_{ context }
    , confirmDialog_{ context }
    , flags_{ flags | ImGuiWindowFlags_NoCollapse } {

    name_.Append("###");
    name_.Append(id);

    saveDialog_.Connect<SaveDialog::SaveEvent, &EditorWindow::OnSave>(this);
    saveDialog_.Connect<SaveDialog::CancelEvent, &EditorWindow::OnCancel>(this);
    saveDialog_.Connect<SaveDialog::CloseEvent, &EditorWindow::OnClose>(this);

    confirmDialog_.Connect<ConfirmDialog::YesEvent, &EditorWindow::OnConfirmYes>(this);
    confirmDialog_.Connect<ConfirmDialog::NoEvent, &EditorWindow::OnConfirmNo>(this);
}

void EditorWindow::Render() {
    if (!Valid()) {
        return;
    }

    ImGui::SetNextWindowSize(DefaultSize(), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(name_.Data(), &visible_, flags_ | (modified_ ? ImGuiWindowFlags_UnsavedDocument : 0))) {
        if (!visible_) {
            if (modified_) {
                closeOnSave_ = true;
                visible_ = true;
                context_.Events().ShowModal(&saveDialog_);
            } else {
                Close();
            }
        }

        Toolbar();
        Content();
    }
    ImGui::End();
}

void EditorWindow::SetModified(bool modified) {
    modified_ = modified;
}

void EditorWindow::OnConfirmYes() {
    context_.Events().CloseModal(&confirmDialog_);

    Reload();
    SetModified(false);
}

void EditorWindow::OnConfirmNo() {
    context_.Events().CloseModal(&confirmDialog_);
}

void EditorWindow::OnSave() {
    context_.Events().CloseModal(&saveDialog_);

    if (Save()) {
        SetModified(false);

        if (closeOnSave_) {
            closeOnSave_ = false;
            Close();
        }
    } else {
        context_.Events().Error("Failed to save asset");
    }
}

void EditorWindow::OnClose() {
    context_.Events().CloseModal(&saveDialog_);

    if (modified_) {
        SetModified(false);
        Reload();
    }

    if (closeOnSave_) {
        closeOnSave_ = false;
        Close();
    }
}

void EditorWindow::OnCancel() {
    context_.Events().CloseModal(&saveDialog_);
}

void EditorWindow::Close() {
    visible_ = false;
    Closed();
}

void EditorWindow::Toolbar() {
    const auto size{ ImGui::GetContentRegionAvail() };

    if (ImGui::BeginChild("##toolbar", ImVec2{ size.x, ImGui::GetFontSize() + 2 * ImGui::GetStyle().FramePadding.y })) {
        {
            ImScope::Disabled disabled{ !modified_ };

            if (ImGui::Button(ICON_FA_RELOAD)) {
                confirmDialog_.SetTitle("Reload?");
                confirmDialog_.SetText("Revert all changes?");
                context_.Events().ShowModal(&confirmDialog_);
            }
        }

        ImGui::SameLine();

        {
            ImScope::Disabled disabled{ !modified_ };

            if (ImGui::Button(ICON_FA_CONTENT_SAVE)) {
                if (Save()) {
                    SetModified(false);
                }
            }
        }
    }

    ImGui::EndChild();
}

void EditorWindow::ConfirmDialog::Show() {
    ImGui::TextUnformatted(text_.Data());

    if (ImGui::Button("Yes")) {
        Emit(YesEvent{});
    }

    ImGui::SameLine();

    if (ImGui::Button("No")) {
        Emit(NoEvent{});
    }
}

void EditorWindow::SaveDialog::Show() {
    ImGui::TextUnformatted("Save?");

    if (ImGui::Button("Yes")) {
        Emit(SaveEvent{});
    }

    ImGui::SameLine();

    if (ImGui::Button("No")) {
        Emit(CloseEvent{});
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        Emit(CancelEvent{});
    }
}

} // namespace ugine::ed