#include "Math.h"

#include <glm/gtc/quaternion.hpp>

#include <list>
#include <random>

namespace {

f32 angle(const glm::vec2& a, const glm::vec2& b) {
    const auto dot{ a.x * b.x + a.y * b.y };
    const auto det{ a.x * b.y - a.y * b.x };
    return atan2(det, dot);
}

} // namespace

namespace ugine {

void TurnVector(glm::vec3& vec, Axis up) {
    switch (up) {
    case Axis::XPos:
        std::swap(vec.x, vec.y);
        vec.x *= -1;
        break;
    case Axis::XNeg:
        std::swap(vec.x, vec.y);
        vec.y *= -1;
        break;
    case Axis::YPos:
        // No-op.
        break;
    case Axis::YNeg:
        vec.x *= -1;
        vec.y *= -1;
        break;
    case Axis::ZPos:
        std::swap(vec.y, vec.z);
        vec.z *= -1;
        break;
    case Axis::ZNeg:
        std::swap(vec.y, vec.z);
        vec.y *= -1;
        break;
    }
}

f32 UnwindDegrees(f32 a) {
    while (a > 180.f) {
        a -= 360.f;
    }
    while (a < -180.f) {
        a += 360.f;
    }
    return a;
}

void UnwindDegrees(glm::vec3& a) {
    a.x = UnwindDegrees(a.x);
    a.y = UnwindDegrees(a.y);
    a.z = UnwindDegrees(a.z);
}

glm::vec3 QuatToEuler(const glm::fquat& q) {
    glm::vec3 result;
    const f32 sqw = q.w * q.w;
    const f32 sqx = q.x * q.x;
    const f32 sqy = q.y * q.y;
    const f32 sqz = q.z * q.z;
    const f32 unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
    const f32 test = q.x * q.w - q.y * q.z;

    if (test > 0.499995f * unit) {
        // singularity at north pole

        // yaw pitch roll
        result.y = 2.0f * atan2f(q.y, q.x);
        result.x = math::PI_OVER_2;
        result.z = 0;
    } else if (test < -0.499995f * unit) {
        // singularity at south pole

        // yaw pitch roll
        result.y = -2.0f * atan2f(q.y, q.x);
        result.x = -math::PI_OVER_2;
        result.z = 0;
    } else {
        // yaw pitch roll
        const glm::fquat tq{ q.w, q.z, q.x, q.y };
        result.y = atan2f(2.0f * tq.x * tq.w + 2.0f * tq.y * tq.z, 1 - 2.0f * (tq.z * tq.z + tq.w * tq.w));
        result.x = asinf(2.0f * (tq.x * tq.z - tq.w * tq.y));
        result.z = atan2f(2.0f * tq.x * tq.y + 2.0f * tq.z * tq.w, 1 - 2.0f * (tq.y * tq.y + tq.z * tq.z));
    }

    result = glm::degrees(result);
    UnwindDegrees(result);
    return result;
}

glm::fquat EulerToQuat(const glm::vec3& v) {
    return glm::fquat(v);
}

glm::vec3 RandomUniformVector() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f32> dis(0.0, 1.0);
    return glm::vec3{ dis(gen), dis(gen), dis(gen) };
}

u32 Random(u32 from, u32 to) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int<u32> dis(from, to);
    return dis(gen);
}

f32 RandomFloat() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f32> dis(0.0, 1.0);
    return dis(gen);
}

glm::vec3 SphericalToCartesian(f32 rho, f32 theta, f32 phi) {
    glm::vec3 coord{};
    coord.x = rho * sinf(phi) * sinf(theta);
    coord.y = rho * cosf(phi);
    coord.z = rho * sinf(phi) * cosf(theta);
    return coord;
}

} // namespace ugine
