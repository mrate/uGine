#include "GameViewport.h"
#include "../EditorContext.h"

#include "../widgets/ImGuiEx.h"
#include "../widgets/ImGuiScope.h"

#include <ugine/engine/gfx/GraphicsScene.h>

namespace ugine::ed {

GameViewport::GameViewport(EditorContext& context)
    : EditorTool{ context }
    , gfxState_{ context_.Engine().GetState<GraphicsState>() } {
}

void GameViewport::Render() {
    ImScope::StyleVar padding{ ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 } };

    if (ImGui::Begin(ICON_FA_CONTROLLER_CLASSIC " Game view", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if (auto world{ context_.ActiveWorld() }; world) {
            auto gfxScene{ world->GetScene<GraphicsScene>() };

            const auto viewportSize{ ImGui::GetContentRegionAvail() };
            const auto sceneSize{ viewportSize };

            bool updateCamera{};
            if (sceneSize.x != sceneSize_.x || sceneSize.y != sceneSize_.y) {
                sceneSize_ = sceneSize;
                updateCamera = true;
            }

            for (auto&& [ent, camera] : world->Registry().view<CameraComponent>().each()) {
                if (!camera.isMain) {
                    continue;
                }

                auto cameraGO{ world->Get(ent) };
                if (updateCamera) {
                    cameraGO.Patch<CameraComponent>([&](CameraComponent& c) {
                        c.width = u32(sceneSize.x);
                        c.height = u32(sceneSize.y);
                    });
                }

                ImGui::Image(ImGuiTextureHandle(gfxScene->GetCameraRtv(cameraGO), gfxState_->device), sceneSize);
                break;
            }
        } else {
            ImGui::Text("No world, no fun :-(");
        }
    }

    ImGui::End();
}

} // namespace ugine::ed