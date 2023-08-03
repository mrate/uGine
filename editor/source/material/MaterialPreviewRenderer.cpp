#include "MaterialPreviewRenderer.h"
#include "../EditorContext.h"

#include <ugine/engine/gfx/ImGui.h>

namespace ugine::ed {

using namespace ugine::gfxapi;

void MaterialPreviewRenderer::Resize(u32 width, u32 height) {
    if (width_ == width && height_ == height) {
        return;
    }

    width_ = width;
    height_ = height;

    preview_.Resize(width_, height_);
    preview_.SetRendering(true);
}

void MaterialPreviewRenderer::SetMaterial(ResourceHandle<Material> material) {
    preview_.SetMaterial(material);

    theta_ = 0.0f;
    phi_ = 90.0f;

    UpdateCamera();
}

void MaterialPreviewRenderer::Render() {
    if (controlling_) {
        const auto delta{ ImGui::GetMouseDragDelta(0) };

        theta_ += (delta.x - delta_.x) / 2.0f;
        phi_ += (delta.y - delta_.y) / 2.0f;
        phi_ = std::max(1.0f, std::min(179.0f, phi_));

        delta_ = delta;

        UpdateCamera();
    }

    auto output{ preview_.GetOutput() };

    const auto image{ ImGui::GetCursorScreenPos() };
    ImGui::Image(ImGuiTextureHandle(output, context_.GfxDevice()),
        ImVec2{ float(width_), float(height_) } /*, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1)*/);

    if (ImGui::IsWindowFocused()) {
        if (ImGui::IsMouseClicked(0)) {
            const auto mouse{ ImGui::GetMousePos() };
            if (mouse.x >= image.x && mouse.y >= image.y && mouse.x <= image.x + width_ && mouse.y <= image.y + height_) {
                controlling_ = true;
                delta_ = ImVec2{};
            }
        }

        if (controlling_ && ImGui::IsMouseReleased(0)) {
            controlling_ = false;
        }
    }
}

void MaterialPreviewRenderer::SetRendering(bool rendering) {
    preview_.SetRendering(rendering);
}

void MaterialPreviewRenderer::UpdateCamera() {
    const auto cameraPosition{ SphericalToCartesian(1.0f, glm::radians(theta_), glm::radians(phi_)) };
    preview_.SetCameraTransform(Transformation{ glm::inverse(glm::lookAtRH(cameraPosition, glm::vec3{}, -math::UP)) });
}

} // namespace ugine::ed