#include "Hash.h"

namespace std {

size_t hash<glm::vec3>::operator()(const glm::vec3& vec) const noexcept {
    size_t res{ 0 };
    ugine::HashCombine(res, vec.x, vec.y, vec.z);
    return res;
}

size_t hash<glm::vec4>::operator()(const glm::vec4& vec) const noexcept {
    size_t res{ 0 };
    ugine::HashCombine(res, vec.x, vec.y, vec.z, vec.w);
    return res;
}

size_t hash<glm::mat3>::operator()(const glm::mat3& mat) const noexcept {
    size_t res{ 0 };
    for (int i = 0; i < 3; ++i) {
        ugine::HashCombine(res, mat[i]);
    }
    return res;
}

size_t hash<glm::mat4>::operator()(const glm::mat4& mat) const noexcept {
    size_t res{ 0 };
    for (int i = 0; i < 4; ++i) {
        ugine::HashCombine(res, mat[i]);
    }
    return res;
}

} // namespace std
