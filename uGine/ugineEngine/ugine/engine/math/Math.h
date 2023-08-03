#pragma once

#include <ugine/Ugine.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <vector>

namespace ugine {

namespace math {
    constexpr f32 PI{ 3.1415926535897932f };
    constexpr f32 TWO_PI{ 6.28318530718f };
    constexpr f32 PI_INV{ 0.31830988618f };
    constexpr f32 PI_OVER_2{ 1.57079632679f };
    constexpr f32 PI_OVER_4{ 0.78539816339f };
    constexpr f32 PI_HALF{ PI_OVER_2 };
    constexpr f32 EPSILON{ 1e-8f };

    constexpr glm::vec3 FORWARD{ 0.0f, 0.0f, -1.0f };
    constexpr glm::vec3 UP{ 0.0f, 1.0f, 0.0f };
} // namespace math

enum class Axis {
    XPos,
    XNeg,
    YPos,
    YNeg,
    ZPos,
    ZNeg,
};

inline u32 CalculateMipLevels(u32 width, u32 height) {
    return static_cast<u32>(std::floor(std::log2(std::max(width, height)))) + 1;
}

inline f32 Max(const glm::vec3& vec) {
    return std::max(std::max(vec.x, vec.y), vec.z);
}

inline f32 Min(const glm::vec3& vec) {
    return std::min(std::min(vec.x, vec.y), vec.z);
}

void TurnVector(glm::vec3& vec, Axis up);

inline glm::fquat LookAtDir(const glm::vec3& forward, const glm::vec3& up = math::UP) {
    return glm::quatLookAtRH(glm::normalize(forward), up);
}

inline glm::fquat LookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up = math::UP) {
    return glm::lookAtRH(eye, center, up);
}

glm::vec3 QuatToEuler(const glm::fquat& q);
glm::fquat EulerToQuat(const glm::vec3& v);

inline glm::vec3 Interpolate(const glm::vec3& a, const glm::vec3& b, f32 p) {
    return glm::mix(a, b, p);
}

inline glm::fquat Interpolate(const glm::fquat& a, const glm::fquat& b, f32 p) {
    return glm::slerp(a, b, p);
}

glm::vec3 RandomUniformVector();
u32 Random(u32 from, u32 to);
f32 RandomFloat();

glm::vec3 SphericalToCartesian(f32 rho, f32 theta, f32 phi);

} // namespace ugine
