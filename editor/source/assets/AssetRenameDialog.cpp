#include "AssetRenameDialog.h"

#include "../EditorContext.h"

namespace ugine::ed {

void AssetRenameDialog::Init() {
    SetRightButton(Button{
        .enabled = true,
        .name = "OK",
        .func =
            [this] {
                if (context_.MoveResource(newPath_ / newName_.Data(), id_)) {
                    context_.Events().CloseModal(this);
                }
            },
    });
}

void AssetRenameDialog::Show() {
    BeginContent();

    ImGui::Text("File name:");
    ImGui::SameLine();
    ImGui::InputText("##filename", newName_.Data(), newName_.SIZE);

    EndContent();

    Buttons();
}

void AssetRenameDialog::Rename(const ResourceID& id) {
    id_ = id;

    const auto path{ context_.Engine().GetResources().ResourcePath(id) };
    newName_ = path.String();
    newPath_ = path.ParentPath();

    SetRightButtonEnabled(0, !path.Empty());
}

} // namespace ugine::ed