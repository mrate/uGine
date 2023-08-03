#include "SaveWindow.h"

#include <ugine/StringUtils.h>

#include "../EditorContext.h"
#include "../widgets/ImGuiEx.h"
#include "../widgets/PropertyTable.h"

namespace ugine::ed {

void SaveWindow::Init() {
    SetRightButtons({ Button{
        .enabled = false,
        .name = ICON_FA_CONTENT_SAVE " Save",
        .func =
            [this] {
                try {
                    saveAction_(targetPath_, fileName_);
                    context_.Events().CloseModal(this);
                } catch (const std::exception& ex) {
                    Emit(ErrorEvent{ std::format("Failed to save file '{}': {}", (targetPath_ / fileName_).String(), ex.what()).c_str() });
                }
            },
    } });
}

void SaveWindow::Show() {
    BeginContent();
    {
        PropertyTable table{ "##save_world", &context_ };
        table.EditProperty("Name:", fileName_);
    }

    context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);
    EndContent();

    SetRightButtonEnabled(0, !fileName_.Empty());
    Buttons();
}

} // namespace ugine::ed