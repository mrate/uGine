#pragma once

#include "Transformation.h"

#include <ugine/Json.h>
#include <ugine/engine/math/Frustum.h>

#include <glm/glm.hpp>

#include <memory>

namespace ugine {

class GameObject;

glm::mat4 ViewMatrixFromTransformation(const Transformation& trans, bool up = true);

class Camera {
public:
    enum class ProjectionType {
        Ortho,
        Perspective,
    };

    Camera() = default;
    Camera(f32 left, f32 top, f32 right, f32 bottom, f32 nearZ, f32 farZ);
    Camera(f32 vFovDeg, f32 width, f32 height, f32 nearZ, f32 farZ);
    Camera(ProjectionType type, const glm::mat4& projectionMatrix, f32 nearZ, f32 farZ);

    static Camera Ortho(f32 left, f32 top, f32 right, f32 bottom, f32 nearZ, f32 farZ);
    static Camera Perspective(f32 vFovDeg, f32 width, f32 height, f32 nearZ, f32 farZ);
    static Camera Perspective(f32 leftTan, f32 rightTan, f32 topTan, f32 bottomTan, f32 nearZ, f32 farZ);

    ProjectionType Type() const { return type_; }

    const glm::mat4& ProjectionMatrix() const { return projectionMatrix_; }

    void SetSize(f32 width, f32 height);

    const Frustum& GetFrustum() const { return frustum_; }

    f32 Left() const { return left_; }
    f32 Right() const { return right_; }
    f32 Top() const { return top_; }
    f32 Bottom() const { return bottom_; }
    f32 Fov() const { return fov_; }
    f32 Width() const { return width_; }
    f32 Height() const { return height_; }
    f32 Near() const { return near_; }
    f32 Far() const { return far_; }

    void FromJson(const nlohmann::json& json);
    void ToJson(nlohmann::json& json) const;

private:
    void UpdateMatrix();
    void UpdateFrustum();
    void UpdateHash();

    ProjectionType type_{ ProjectionType::Perspective };
    glm::mat4 projectionMatrix_{};
    Frustum frustum_{};

    f32 left_{};
    f32 right_{};
    f32 top_{};
    f32 bottom_{};
    f32 fov_{};
    f32 width_{};
    f32 height_{};
    f32 near_{};
    f32 far_{};
};

} // namespace ugine

UGINE_SERIALIZABLE(ugine::Camera);
