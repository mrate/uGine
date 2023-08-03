#pragma once

#include "Types.h"

#include <functional>
#include <string_view>

namespace ugine::gfxapi {

template <typename T> inline bool TrivialCmp(const T& a, const T& b) {
    return std::memcmp(&a, &b, sizeof(T)) == 0;
}

template <typename T> inline std::size_t TrivialHash(const T& a) {
    return std::hash<std::string_view>{}(std::string_view{ reinterpret_cast<const char*>(&a), sizeof(T) });
}

bool operator==(const CompiledShader& a, const CompiledShader& b);
inline bool operator!=(const CompiledShader& a, const CompiledShader& b) {
    return !(a == b);
}

inline bool operator==(const BlendDesc& a, const BlendDesc& b) {
    return TrivialCmp(a, b);
}

inline bool operator!=(const BlendDesc& a, const BlendDesc& b) {
    return !(a == b);
}

inline bool operator==(const DepthStencilDesc& a, const DepthStencilDesc& b) {
    return TrivialCmp(a, b);
}

inline bool operator!=(const DepthStencilDesc& a, const DepthStencilDesc& b) {
    return !(a == b);
}

inline bool operator==(const RasterizerDesc& a, const RasterizerDesc& b) {
    return TrivialCmp(a, b);
}

inline bool operator!=(const RasterizerDesc& a, const RasterizerDesc& b) {
    return !(a == b);
}

} // namespace ugine::gfxapi

template <> struct std::hash<ugine::gfxapi::CompiledShader> {
    std::size_t operator()(const ugine::gfxapi::CompiledShader& shader) const noexcept;
};

template <> struct std::hash<ugine::gfxapi::BlendDesc> {
    std::size_t operator()(const ugine::gfxapi::BlendDesc& desc) const noexcept { return ugine::gfxapi::TrivialHash(desc); }
};

template <> struct std::hash<ugine::gfxapi::DepthStencilDesc> {
    std::size_t operator()(const ugine::gfxapi::DepthStencilDesc& desc) const noexcept { return ugine::gfxapi::TrivialHash(desc); }
};

template <> struct std::hash<ugine::gfxapi::RasterizerDesc> {
    std::size_t operator()(const ugine::gfxapi::RasterizerDesc& desc) const noexcept { return ugine::gfxapi::TrivialHash(desc); }
};
