#include "MaterialInstanceWindow.h"

#include "MaterialTool.h"

#include "../EditorContext.h"
#include "../widgets/ImGuiEx.h"
#include "../widgets/PropertyTable.h"

#include <ugine/engine/gfx/asset/SerializedMaterial.h>
#include <ugine/engine/world/WorldManager.h>
#include <ugine/engine/world/WorldSerializer.h>

namespace ugine::ed {

void MaterialInstanceWindow::Init() {
    SetRightButtons({ Button{
        .enabled = true,
        .name = ICON_FA_FOLDER " Save",
        .func =
            [this] {
                try {
                    auto serialized{ InstantiateMaterial(origin_, fileName_) };
                    context_.CreateResource<Material>(targetPath_, fileName_);
                    // TODO: [resources]
                    // , serialized

                    context_.Events().CloseModal(this);
                } catch (const std::exception& ex) {
                    Emit(ErrorEvent{ std::format("Failed to save material instance: {}", ex.what()).c_str() });
                }
            },
    } });
}

void MaterialInstanceWindow::SetOrigin(ResourceHandle<Material> material, StringView name) {
    origin_ = material;
    fileName_ = name;
}

void MaterialInstanceWindow::Show() {
    BeginContent();

    {
        PropertyTable table{ "##material_instance", &context_ };

        table.EditProperty("Name:", fileName_);
    }

    context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);
    context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);

    EndContent();

    SetRightButtonEnabled(0, !fileName_.Empty());
    Buttons();
}

} // namespace ugine::ed