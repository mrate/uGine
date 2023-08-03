#pragma once

#include <ugine/engine/math/Math.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace ugine {

struct Transformation {
    mutable bool dirty{ true };

    glm::vec3 position{};
    glm::fquat rotation{};
    glm::vec3 scale{};

    Transformation();
    explicit Transformation(const glm::mat4& matrix);
    Transformation(const glm::vec3& position, const glm::fquat& rotation, const glm::vec3& scale);
    static Transformation LookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up = math::UP);

    const glm::mat4& Matrix() const;
    glm::mat4 Inverse() const;

    f32 SquaredDistance(const glm::vec3& to) const {
        const auto diff{ position - to };
        return diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    }

    f32 SquaredDistance(const Transformation& to) const { return SquaredDistance(to.position); }
    f32 Distance(const glm::vec3& to) const { return sqrtf(SquaredDistance(to)); }
    f32 Distance(const Transformation& to) const { return Distance(to.position); }

    glm::vec3 Euler() const { return glm::eulerAngles(rotation); }
    void Euler(const glm::vec3& euler) { rotation = glm::fquat(euler); }

    void Translate(const glm::vec3& t);
    void Translate(const glm::mat4& t);
    void Rotate(const glm::fquat& quat);
    void Rotate(const glm::mat4& rot);
    void Scale(const glm::vec3& s);
    void Scale(const glm::mat4& s);

    Transformation operator+(const Transformation& other) const;
    Transformation operator+(const glm::mat4& other) const;
    Transformation& operator+=(const Transformation& trans);
    Transformation& operator+=(const glm::mat4& trans);

    Transformation operator*(const Transformation& other) const;
    Transformation operator*(const glm::mat4& other) const;
    Transformation& operator*=(const Transformation& trans);
    Transformation& operator*=(const glm::mat4& trans);

    bool operator==(const Transformation& other) const { return position == other.position && rotation == other.rotation && scale == other.scale; }
    bool operator!=(const Transformation& other) const { return !(*this == other); }

private:
    mutable glm::mat4 matrix_{};
};

Transformation operator+(const glm::mat4& m, const Transformation& t);
Transformation operator*(const glm::mat4& m, const Transformation& t);

} // namespace ugine
