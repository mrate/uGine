#include "Raycast.h"
#include "Math.h"

namespace ugine {

Hit RayAabbIntersect(const Ray& ray, const AABB& aabb) {
    // TODO: https://iquilezles.org/articles/intersectors/

    Hit hit{ .ray = ray };

    //UGINE_ASSERT(false && "AABB with origin at 0");
    //glm::vec3 m = 1.0f / ray.dir; // can precompute if traversing a set of aligned boxes
    //glm::vec3 n = m * ray.origin; // can precompute if traversing a set of aligned boxes
    //glm::vec3 k = abs(m) * aabb.Size();
    //glm::vec3 t1 = -n - k;
    //glm::vec3 t2 = -n + k;
    //float tN = std::max(std::max(t1.x, t1.y), t1.z);
    //float tF = std::min(std::min(t2.x, t2.y), t2.z);
    //if (tN > tF || tF < 0.0) {
    //    return hit;
    //}

    //auto outNormal = (tN > 0.0) ? glm::step(glm::vec3(tN), t1) : // ro ouside the box
    //    glm::step(t2, glm::vec3(tF));                            // ro inside the box
    //outNormal *= -glm::sign(ray.dir);
    //hit.hit = true;
    //hit.distance = tN;
    //return hit;

    const glm::vec3 invdir{ 1.0f / ray.dir };
    const int sign[3] = { invdir.x < 0 ? 1 : 0, invdir.y < 0 ? 1 : 0, invdir.z < 0 ? 1 : 0 };

    glm::vec3 bounds[2] = { aabb.Min(), aabb.Max() };

    f32 tmin{ (bounds[sign[0]].x - ray.origin.x) * invdir.x };
    f32 tmax{ (bounds[1 - sign[0]].x - ray.origin.x) * invdir.x };
    f32 tymin{ (bounds[sign[1]].y - ray.origin.y) * invdir.y };
    f32 tymax{ (bounds[1 - sign[1]].y - ray.origin.y) * invdir.y };

    if ((tmin > tymax) || (tymin > tmax)) {
        return hit;
    }
    if (tymin > tmin) {
        tmin = tymin;
    }
    if (tymax < tmax) {
        tmax = tymax;
    }

    f32 tzmin{ (bounds[sign[2]].z - ray.origin.z) * invdir.z };
    f32 tzmax{ (bounds[1 - sign[2]].z - ray.origin.z) * invdir.z };

    if ((tmin > tzmax) || (tzmin > tmax)) {
        return hit;
    }
    if (tzmin > tmin) {
        tmin = tzmin;
    }
    if (tzmax < tmax) {
        tmax = tzmax;
    }

    hit.hit = true;
    hit.distance = tmin;

    return hit;
}

Hit RayPlaneIntersection(const Ray& ray, const glm::vec3& normal, const glm::vec3& position) {
    // TODO: https://iquilezles.org/articles/intersectors/
    Hit hit{ .ray = ray };

    // assuming vectors are all normalized
    const auto denom{ glm::dot(normal, ray.dir) };
    if (denom <= math::EPSILON) {
        return hit;
    }

    const auto p0l0{ position - ray.origin };
    const auto xt{ glm::dot(p0l0, normal) / denom };
    if (xt >= 0) {
        hit.hit = true;
        hit.distance = xt;
    }

    return hit;
}

Hit RaySphereIntersect(const Ray& ray, const glm::vec3& center, f32 radius) {
    // iq
    Hit hit{ .ray = ray };

    const glm::vec3 oc{ ray.origin - center };

    const f32 b{ glm::dot(oc, ray.dir) };
    const f32 c{ glm::dot(oc, oc) - (radius * radius) };
    const f32 h{ b * b - c };
    if (h < 0.0f) {
        return hit;
    }

    hit.hit = true;
    hit.distance = -b - h;
    return hit;
}

Hit RayTriangleIntersect(const Ray& ray, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, bool singleSided) {
    // TODO: https://iquilezles.org/articles/intersectors/

    Hit hit{ .ray = ray };

    const auto v0v1{ v1 - v0 };
    const auto v0v2{ v2 - v0 };

    const auto pvec{ glm::cross(ray.dir, v0v2) };
    const auto det{ glm::dot(v0v1, pvec) };

    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < math::EPSILON) {
        return hit;
    }

    const auto invDet{ 1.0f / det };

    const auto tvec{ ray.origin - v0 };
    const auto u{ glm::dot(tvec, pvec) * invDet };
    if (u < 0 || u > 1) {
        return hit;
    }

    const auto qvec{ glm::cross(tvec, v0v1) };
    const auto v{ dot(ray.dir, qvec) * invDet };
    if (v < 0 || u + v > 1) {
        return hit;
    }

    // Backface culling - counter clock-wise.
    const auto norm{ glm::normalize(glm::cross(v0v1, v0v2)) };
    if (glm::dot(ray.dir, norm) > 0 && singleSided) {
        return hit;
    }

    hit.hit = true;
    hit.distance = glm::dot(v0v2, qvec) * invDet;
    hit.uv = glm::vec2{ u, v };
    return hit;
}

Ray RayFromCamera(const glm::mat4& projection, const glm::mat4& view, f32 x, f32 y, f32 width, f32 height) {
    const glm::vec4 viewport{ 0, 0, width, height };

    const auto rayStart{ glm::unProjectZO(glm::vec3{ x, height - y - 1, 0.0f }, view, projection, viewport) };
    const auto rayEnd{ glm::unProjectZO(glm::vec3{ x, height - y - 1, 1.0f }, view, projection, viewport) };
    const auto dir{ glm::normalize(rayEnd - rayStart) };
    return Ray{
        .origin = rayStart,
        .dir = dir,
        .invDir = 1.0f / dir,
    };
}

} // namespace ugine
