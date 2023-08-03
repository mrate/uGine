#include "Camera.h"

#include <ugine/engine/world/Transformation.h>
//#include <ugine/Hash.h>

#include <ugine/engine/math/Math.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace ugine {

glm::mat4 ViewMatrixFromTransformation(const Transformation& trans, bool up) {
    return up ? glm::inverse(trans.Matrix()) : glm::lookAtRH(trans.position, trans.position + trans.rotation * math::FORWARD, trans.rotation * math::UP);
}

Camera Camera::Ortho(f32 left, f32 top, f32 right, f32 bottom, f32 nearZ, f32 farZ) {
    return Camera(left, top, right, bottom, nearZ, farZ);
}

Camera Camera::Perspective(f32 vFovDeg, f32 width, f32 height, f32 nearZ, f32 farZ) {
    return Camera(vFovDeg, width, height, nearZ, farZ);
}

Camera Camera::Perspective(f32 leftTan, f32 rightTan, f32 topTan, f32 bottomTan, f32 nearZ, f32 farZ) {
    const auto projectionMatrix{ glm::frustum(leftTan * nearZ, rightTan * nearZ, bottomTan * nearZ, topTan * nearZ, nearZ, farZ) };
    return Camera(ProjectionType::Perspective, projectionMatrix, nearZ, farZ);
}

void Camera::SetSize(f32 width, f32 height) {
    if (type_ == ProjectionType::Perspective) {
        width_ = width;
        height_ = height;
    } else {
        right_ = left_ + width;
        top_ = bottom_ + height;
    }

    UpdateMatrix();
}

void Camera::FromJson(const nlohmann::json& json) {
    // TODO:
}

void Camera::ToJson(nlohmann::json& json) const {
    // TODO:
}

void Camera::UpdateMatrix() {
    if (type_ == ProjectionType::Perspective) {
        projectionMatrix_ = glm::perspectiveFovRH(glm::radians(fov_), width_, height_, near_, far_);
    } else {
        projectionMatrix_ = glm::ortho(left_, right_, bottom_, top_, near_, far_);
    }
    UpdateFrustum();
}

Camera::Camera(f32 left, f32 top, f32 right, f32 bottom, f32 nearZ, f32 farZ)
    : left_(left)
    , top_(top)
    , right_(right)
    , bottom_(bottom)
    , near_(nearZ)
    , far_(farZ)
    , type_(ProjectionType::Ortho) {
    projectionMatrix_ = glm::ortho(left, right, bottom, top, nearZ, farZ);
    UpdateFrustum();
}

Camera::Camera(f32 vFovDeg, f32 width, f32 height, f32 nearZ, f32 farZ)
    : fov_(vFovDeg)
    , width_(width)
    , height_(height)
    , near_(nearZ)
    , far_(farZ)
    , type_(ProjectionType::Perspective) {
    projectionMatrix_ = glm::perspectiveFovRH(glm::radians(vFovDeg), width, height, nearZ, farZ);
    UpdateFrustum();
}

Camera::Camera(ProjectionType type, const glm::mat4& projectionMatrix, f32 nearZ, f32 farZ)
    : type_(type)
    , projectionMatrix_(projectionMatrix)
    , near_(nearZ)
    , far_(farZ) {
    UpdateFrustum();
}

void Camera::UpdateFrustum() {
    enum {
        LEFT = 0,
        RIGHT,
        TOP,
        BOTTOM,
        BACK,
        FRONT,
    };

    // TODO:
    auto matrix{ ProjectionMatrix() /** ViewMatrix()*/ };
    UpdateHash();

    frustum_.planes[LEFT].x = matrix[0].w + matrix[0].x;
    frustum_.planes[LEFT].y = matrix[1].w + matrix[1].x;
    frustum_.planes[LEFT].z = matrix[2].w + matrix[2].x;
    frustum_.planes[LEFT].w = matrix[3].w + matrix[3].x;

    frustum_.planes[RIGHT].x = matrix[0].w - matrix[0].x;
    frustum_.planes[RIGHT].y = matrix[1].w - matrix[1].x;
    frustum_.planes[RIGHT].z = matrix[2].w - matrix[2].x;
    frustum_.planes[RIGHT].w = matrix[3].w - matrix[3].x;

    frustum_.planes[TOP].x = matrix[0].w - matrix[0].y;
    frustum_.planes[TOP].y = matrix[1].w - matrix[1].y;
    frustum_.planes[TOP].z = matrix[2].w - matrix[2].y;
    frustum_.planes[TOP].w = matrix[3].w - matrix[3].y;

    frustum_.planes[BOTTOM].x = matrix[0].w + matrix[0].y;
    frustum_.planes[BOTTOM].y = matrix[1].w + matrix[1].y;
    frustum_.planes[BOTTOM].z = matrix[2].w + matrix[2].y;
    frustum_.planes[BOTTOM].w = matrix[3].w + matrix[3].y;

    frustum_.planes[BACK].x = matrix[0].w + matrix[0].z;
    frustum_.planes[BACK].y = matrix[1].w + matrix[1].z;
    frustum_.planes[BACK].z = matrix[2].w + matrix[2].z;
    frustum_.planes[BACK].w = matrix[3].w + matrix[3].z;

    frustum_.planes[FRONT].x = matrix[0].w - matrix[0].z;
    frustum_.planes[FRONT].y = matrix[1].w - matrix[1].z;
    frustum_.planes[FRONT].z = matrix[2].w - matrix[2].z;
    frustum_.planes[FRONT].w = matrix[3].w - matrix[3].z;

    for (auto i = 0; i < frustum_.planes.size(); i++) {
        glm::normalize(frustum_.planes[i]);
    }
}

void Camera::UpdateHash() {
    /*hash_ = 0;
    utils::HashCombine(hash_, projectionMatrix_);
    utils::HashCombine(hash_, viewMatrix_);*/
}

} // namespace ugine
