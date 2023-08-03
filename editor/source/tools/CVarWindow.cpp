#include "CVarWindow.h"
#include "../EditorContext.h"
#include "../widgets/ImGuiEx.h"

#include <ugine/engine/engine/CVars.h>

namespace ugine::ed {

const char* CVarsVisibleSettings{ "cvars-visible" };

CVarWindow::CVarWindow(EditorContext& context)
    : EditorTool{ context, context.Settings().Get<bool>(CVarsVisibleSettings, false) } {

    auto& debugMenu{ context_.MainMenu().Get("Debug") };
    debugMenu.AddAction([this] { return ImGui::Checkbox("CVars", &visible_); }, [this] { context_.Settings().Set<bool>(CVarsVisibleSettings, visible_); });
}

void CVarWindow::Render() {
    if (ImGui::Begin("CVars", &visible_)) {
        if (ImGui::BeginTabBar("##cvars")) {
            for (auto&& [_, var] : CVars::All()) {
                if (ImGui::BeginTabItem(var.category)) {
                    ImGuiEx::CVarEdit(var);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

namespace {
    EditorToolset::Register _{ [](EditorContext& context) { context.RegisterEditorTool(MakeUnique<CVarWindow>(context.Allocator(), context)); } };
} // namespace

} // namespace ugine::ed