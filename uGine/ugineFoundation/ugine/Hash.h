#pragma once

#include <ugine/Span.h>
#include <ugine/Ugine.h>

#include <glm/glm.hpp>

#include <functional>

namespace std {

template <> struct hash<glm::vec3> {
    size_t operator()(const glm::vec3& vec) const noexcept;
};
template <> struct hash<glm::vec4> {
    size_t operator()(const glm::vec4& vec) const noexcept;
};
template <> struct hash<glm::mat3> {
    size_t operator()(const glm::mat3& mat) const noexcept;
};
template <> struct hash<glm::mat4> {
    size_t operator()(const glm::mat4& mat) const noexcept;
};

} // namespace std

namespace ugine {

inline void HashCombine(std::size_t& seed) {
}

template <typename T, typename... Rest> inline void HashCombine(std::size_t& seed, const T& v, Rest... rest) noexcept {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    HashCombine(seed, rest...);
}

template <typename T> UGINE_FORCE_INLINE constexpr u64 fnv1a(const T* data, size_t size) noexcept {
    constexpr u64 OFFSET{ 14695981039346656037ull };
    constexpr u64 PRIME{ 1099511628211ull };

    u64 hash{ OFFSET };
    for (size_t i{}; i < size; ++i) {
        hash = (hash ^ data[i]) * PRIME;
    }
    return hash;
}

} // namespace ugine
