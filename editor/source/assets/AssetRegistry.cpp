#include "AssetRegistry.h"

#include <format>

namespace ugine::ed {

AssetRegistry::AssetRegistry(EditorContext& context)
    : EditorTool{ context, false } {

    auto& view{ context_.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Assets registry", &visible_); });
}

void AssetRegistry::Render() {
    if (ImGui::Begin(ICON_FA_DATABASE " Assets registry", &visible_)) {
        for (auto [id, ref] : context_.Engine().GetResources().RegisteredResources()) {
            ImGui::Text("UUID: %s", id.ToString().Data());
            ImGui::Text("PATH: %s", ref.path.String().Data());
            ImGui::Text("TYPE: %s", ref.type.Name());
            ImGui::Separator();
        }
    }

    ImGui::End();
}

} // namespace ugine::ed