#include "CameraPreview.h"

#include "../EditorContext.h"

#include <ugine/Profile.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/world/Component.h>

namespace ugine::ed {

CameraPreview::CameraPreview(EditorContext& context)
    : EditorTool{ context, true } {
    auto& view{ context.MainMenu().Get("View") };

    view.AddAction([this] { return ImGui::Checkbox("Camera preview", &visible_); });
}

void CameraPreview::Render() {
    if (!context_.SelectedGO() || !context_.SelectedGO().Has<CameraComponent>()) {
        return;
    }

    auto world{ context_.ActiveWorld() };
    auto gfxScene{ world->GetScene<GraphicsScene>() };

    PROFILE_EVENT_NC("CameraPreview", COLOR_EDITOR_PROFILE);

    auto rtv{ gfxScene->GetCameraRtv(context_.SelectedGO()) };
    if (!rtv) {
        return;
    }

    if (ImGui::Begin(ICON_FA_CAMERA " Camera preview", &visible_, ImGuiWindowFlags_NoCollapse)) {
        const ImVec2 size{ ImGui::GetWindowContentRegionMax() };
        ImGui::Image(ImGuiTextureHandle(rtv, context_.GfxDevice()), size);
    }
    ImGui::End();
}

} // namespace ugine::ed