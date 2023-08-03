#pragma once

#include <ugine/engine/world/Camera.h>
#include <ugine/engine/world/GameObject.h>

#include <optional>

namespace ImGuiEx {

class TransformationWidget {
public:
    enum class Operation {
        Translate,
        Rotate,
        Scale,
        Bounds,
    };

    void SetOperation(Operation operation) { operation_ = operation; }
    Operation GetOperation() const { return operation_; }

    void SetLocal(bool local) { local_ = local; }
    bool IsLocal() const { return local_; }

    bool Render(glm::mat4& deltaTransformation, const glm::mat4& projectionMatrix, const ugine::Transformation& cameraTransformation,
        ugine::Transformation& transformation, std::optional<glm::vec3> snap);
    bool WantCapture() const;
    bool IsActive() const;

private:
    Operation operation_{ Operation::Translate };
    bool local_{};
};

} // namespace ImGuiEx