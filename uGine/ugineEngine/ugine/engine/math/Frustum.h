#pragma once

#include <ugine/Ugine.h>

#include <glm/glm.hpp>

#include <array>

namespace ugine {

class Plane {
public:
    Plane() {}
    Plane(const glm::vec3& orig, const glm::vec3& normal)
        : normal{ normal }
        , d{ -glm::dot(normal, orig) } {}

    Plane(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
        normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
        d = -glm::dot(normal, v1);
    }

    float Distance(const glm::vec3& point) const { return normal.x * point.x + normal.y * point.y + normal.z * point.z + d; }

    glm::vec3 normal{};
    f32 d{};
};

struct Frustum {
    std::array<glm::vec4, 6> planes;
};

} // namespace ugine
