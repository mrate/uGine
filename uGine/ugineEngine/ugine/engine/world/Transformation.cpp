#include "Transformation.h"

#include <ugine/Log.h>
#include <ugine/Ugine.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace ugine {

Transformation::Transformation()
    : position{ 0, 0, 0 }
    , rotation{ 1, 0, 0, 0 }
    , scale{ 1, 1, 1 } {
}

Transformation::Transformation(const glm::mat4& matrix) {
    glm::vec3 skew;
    glm::vec4 persp;
    glm::decompose(matrix, scale, rotation, position, skew, persp);
}

Transformation::Transformation(const glm::vec3& position, const glm::fquat& rotation, const glm::vec3& scale)
    : position{ position }
    , rotation{ rotation }
    , scale{ scale } {
}

glm::mat4 Transformation::Inverse() const {
    return glm::inverse(Matrix());
}

Transformation Transformation::LookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
    return Transformation{ glm::lookAtRH(eye, center, up) };
}

void Transformation::Translate(const glm::vec3& t) {
    position += t;
}

void Transformation::Translate(const glm::mat4& t) {
    position += glm::vec3(t[3]);
}

void Transformation::Rotate(const glm::fquat& quat) {
    rotation = glm::normalize(quat * rotation);
}

void Transformation::Rotate(const glm::mat4& rot) {
    rotation = glm::normalize(glm::fquat(rot) * rotation);
}

void Transformation::Scale(const glm::vec3& s) {
    scale *= s;
}

void Transformation::Scale(const glm::mat4& s) {
    scale *= glm::vec3(s[0][0], s[1][1], s[2][2]);
}

Transformation Transformation::operator+(const Transformation& other) const {
    Transformation t{ *this };

    t.Translate(other.position);
    t.Rotate(other.rotation);
    t.Scale(other.scale);

    return t;
}

Transformation operator+(const glm::mat4& m, const Transformation& t) {
    return Transformation{ m } + t;
}

Transformation& Transformation::operator+=(const Transformation& trans) {
    Translate(trans.position);
    Rotate(trans.rotation);
    Scale(trans.scale);

    return *this;
}

Transformation& Transformation::operator+=(const glm::mat4& trans) {
    return (*this) += Transformation{ trans };
}

Transformation Transformation::operator*(const Transformation& other) const {
    Transformation t;

    t.rotation = glm::normalize(rotation * other.rotation);
    t.scale = scale * other.scale;
    t.position = other.position * scale;
    t.position = rotation * t.position;
    t.position += position;

    return t;
}

Transformation operator*(const glm::mat4& m, const Transformation& t) {
    return Transformation{ m } * t;
}

Transformation& Transformation::operator*=(const Transformation& other) {
    Transformation orig{ *this };

    rotation = glm::normalize(orig.rotation * other.rotation);
    scale = orig.scale * other.scale;
    position = other.position * orig.scale;
    position = orig.rotation * position;
    position += orig.position;

    return *this;
}

Transformation& Transformation::operator*=(const glm::mat4& trans) {
    return (*this) *= Transformation{ trans };
}

const glm::mat4& Transformation::Matrix() const {
    if (dirty) {
        // TODO:
        //dirty = false;
        matrix_ = glm::translate(position) * glm::mat4(rotation) * glm::scale(scale);
    }
    return matrix_;
}

} // namespace ugine
