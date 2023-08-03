#pragma once

#include <ugine/Span.h>
#include <ugine/Ugine.h>

#include <glm/glm.hpp>

namespace ugine {

class AABB {
public:
    AABB() noexcept = default;

    AABB(const glm::vec3& min, const glm::vec3& max)
    noexcept
        : min_{ min }
        , max_{ max } {}

    template <typename T>
    AABB(Span<T> vert)
    noexcept
        : AABB{ vert.Data(), vert.Size() } {}

    template <typename T> AABB(const T* v, size_t count) {
        UGINE_ASSERT(count > 0);

        min_ = max_ = glm::vec3(v[0].x, v[0].y, v[0].z);

        for (size_t i{ 1 }; i < count; ++i) {
            min_.x = std::min(min_.x, v[i].x);
            min_.y = std::min(min_.y, v[i].y);
            min_.z = std::min(min_.z, v[i].z);

            max_.x = std::max(max_.x, v[i].x);
            max_.y = std::max(max_.y, v[i].y);
            max_.z = std::max(max_.z, v[i].z);
        }
    }

    AABB Transform(const glm::mat4& mat) const {
        glm::vec3 v[] = {
            mat * glm::vec4(min_.x, min_.y, min_.z, 1.0f),
            mat * glm::vec4(min_.x, min_.y, max_.z, 1.0f),
            mat * glm::vec4(min_.x, max_.y, min_.z, 1.0f),
            mat * glm::vec4(min_.x, max_.y, max_.z, 1.0f),
            mat * glm::vec4(max_.x, min_.y, min_.z, 1.0f),
            mat * glm::vec4(max_.x, min_.y, max_.z, 1.0f),
            mat * glm::vec4(max_.x, max_.y, min_.z, 1.0f),
            mat * glm::vec4(max_.x, max_.y, max_.z, 1.0f),
        };
        return AABB{ v, 8 };
    }

    AABB Translate(const glm::vec3& point) const { return AABB{ min_ + point, max_ + point }; }

    AABB Scale(float scale) { return AABB{ min_ * scale, max_ * scale }; }

    void Add(const glm::vec3& point) {
        min_ = glm::min(min_, point);
        max_ = glm::max(max_, point);
    }

    AABB Merge(const AABB& other) const { return AABB{ glm::min(Min(), other.Min()), glm::max(Max(), other.Max()) }; }

    const glm::vec3& Min() const { return min_; }
    const glm::vec3& Max() const { return max_; }

    glm::vec3 CenterPoint() const { return (min_ + max_) * 0.5f; }
    glm::vec3 HalfSize() const { return CenterPoint() - min_; }
    glm::vec3 Size() const { return max_ - min_; }
    float Diagonal() const { return glm::length(max_ - min_); }

private:
    glm::vec3 min_{};
    glm::vec3 max_{};
};

} // namespace ugine
