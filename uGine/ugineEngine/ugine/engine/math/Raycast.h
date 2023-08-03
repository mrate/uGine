#pragma once

#include "Aabb.h"

namespace ugine {

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
    glm::vec3 invDir;

    Ray Transform(const glm::mat4& matrix) const {
        const auto newDir{ glm::normalize(matrix * glm::vec4{ dir, 0.0f }) };
        return Ray{
            .origin = matrix * glm::vec4{ origin, 1.0f },
            .dir = newDir,
            .invDir = 1.0f / newDir,
        };
    }
};

struct Hit {
    Ray ray;
    f32 distance{};
    glm::vec2 uv{};
    bool hit{};
};

struct Sphere {
    glm::vec3 center;
    f32 radius{};
};

Hit RayAabbIntersect(const Ray& ray, const AABB& aabb);
Hit RayPlaneIntersection(const Ray& ray, const glm::vec3& normal, const glm::vec3& position);
Hit RaySphereIntersect(const Ray& ray, const glm::vec3& center, f32 radius);
inline Hit RaySphereIntersect(const Ray& ray, const Sphere& sphere) {
    return RaySphereIntersect(ray, sphere.center, sphere.radius);
}
Hit RayTriangleIntersect(const Ray& ray, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, bool singleSided = true);

Ray RayFromCamera(const glm::mat4& projection, const glm::mat4& view, f32 x, f32 y, f32 width, f32 height);

} // namespace ugine
