#pragma once

#include <ugine/Ugine.h>

#include <glm/glm.hpp>

namespace ugine {

class AABB;
struct Frustum;

inline f32 PointPlaneDistance(const glm::vec3& planeNormal, const glm::vec3& point) {
    return glm::dot(point, planeNormal);
}

f32 PointAABBDistanceSquared(const AABB& aabb, const glm::vec3& point);

bool AabbInFrustum(const Frustum& frustum, const AABB& aabb);

} // namespace ugine
