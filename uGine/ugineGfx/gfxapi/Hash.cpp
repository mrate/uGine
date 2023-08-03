#include <gfxapi/Hash.h>
#include <ugine/Hash.h>

namespace ugine::gfxapi {

bool operator==(const CompiledShader& a, const CompiledShader& b) {
    if (a.size != b.size) {
        return false;
    }

    if (std::strcmp(a.name, b.name) != 0) {
        return false;
    }

    return std::memcmp(a.data, b.data, a.size) == 0;
}

} // namespace ugine::gfxapi

std::size_t std::hash<ugine::gfxapi::CompiledShader>::operator()(const ugine::gfxapi::CompiledShader& shader) const noexcept {
    const auto hashName{ shader.name ? std::hash<std::string_view>{}(std::string_view{ shader.name }) : 0 };
    const auto hashData{ shader.data ? std::hash<std::string_view>{}(std::string_view{ static_cast<const char*>(shader.data), shader.size }) : 0 };

    std::size_t hash{};
    ugine::HashCombine(hash, hashName, hashData);
    return hash;
}
