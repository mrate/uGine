#include "TransformationWidget.h"
#include "ImGuiEx.h"

#include <ugine/engine/gfx/ImGui.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ImGuizmo.h>

namespace ImGuiEx {

using namespace ugine;

bool TransformationWidget::Render(glm::mat4& deltaTransformation, const glm::mat4& projectionMatrix, const Transformation& cameraTransformation,
    Transformation& objectTransformation, std::optional<glm::vec3> snap) {

    const auto view{ cameraTransformation.Inverse() };
    glm::mat4 model{ objectTransformation.Matrix() };

    ImGuizmo::OPERATION op{ ImGuizmo::TRANSLATE };
    switch (operation_) {
        using enum Operation;

    case Translate: op = ImGuizmo::TRANSLATE; break;
    case Rotate: op = ImGuizmo::ROTATE; break;
    case Scale: op = ImGuizmo::SCALE; break;
    case Bounds: op = ImGuizmo::BOUNDS; break;
    }

    float snapValue[3];
    float* snapPtr{};
    if (snap.has_value()) {
        snapValue[0] = snap->x;
        snapValue[1] = snap->y;
        snapValue[2] = snap->z;
        snapPtr = snapValue;
    }

    float deltaMatrix[16];

    if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projectionMatrix), op, local_ ? ImGuizmo::LOCAL : ImGuizmo::WORLD, glm::value_ptr(model),
            deltaMatrix, snapPtr, nullptr, nullptr)) {
        ImGuiEx::FloatArrayToMatrix(deltaMatrix, deltaTransformation);
        return true;
    }

    return false;
}

bool TransformationWidget::WantCapture() const {
    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

bool TransformationWidget::IsActive() const {
    return ImGuizmo::IsUsing();
}

} // namespace ImGuiEx