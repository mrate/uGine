#include "Culling.h"
#include "Aabb.h"
#include "Frustum.h"

#include <glm/glm.hpp>

namespace ugine {

f32 PointAABBDistanceSquared(const AABB& aabb, const glm::vec3& point) {
    const auto a{ aabb.CenterPoint() };
    const auto halfSize{ aabb.HalfSize() };
    const auto dx{ std::max(std::abs(point.x - a.x) - halfSize.x, 0.0f) };
    const auto dy{ std::max(std::abs(point.y - a.y) - halfSize.y, 0.0f) };
    const auto dz{ std::max(std::abs(point.z - a.z) - halfSize.z, 0.0f) };
    return dx * dx + dy * dy + dz * dz;
}

bool AabbInFrustum(const Frustum& frustum, const AABB& aabb) {
    std::array<glm::vec3, 8> points = {
        glm::vec3{ aabb.Min().x, aabb.Min().y, aabb.Min().z },
        glm::vec3{ aabb.Max().x, aabb.Min().y, aabb.Min().z },
        glm::vec3{ aabb.Min().x, aabb.Max().y, aabb.Min().z },
        glm::vec3{ aabb.Max().x, aabb.Max().y, aabb.Min().z },

        glm::vec3{ aabb.Min().x, aabb.Min().y, aabb.Max().z },
        glm::vec3{ aabb.Max().x, aabb.Min().y, aabb.Max().z },
        glm::vec3{ aabb.Min().x, aabb.Max().y, aabb.Max().z },
        glm::vec3{ aabb.Max().x, aabb.Max().y, aabb.Max().z },
    };

    bool inside{};
    for (const auto& plane : frustum.planes) {
        inside = false;
        for (const auto& point : points) {
            if (plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w > 0.0f) {
                inside = true;
                break;
            }
        }

        if (!inside) {
            return false;
        }
    }
    return true;
}

} // namespace ugine
