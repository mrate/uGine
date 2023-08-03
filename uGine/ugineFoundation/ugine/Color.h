#pragma once

#include <ugine/Ugine.h>
#include <ugine/Utils.h>

#include <glm/glm.hpp>

namespace ugine {

struct ColorRGB {
    f32 r{};
    f32 g{};
    f32 b{};

    explicit operator glm::vec3() const { return glm::vec3{ r, g, b }; }
    u32 ToUint() const { return (u32(r * 255) << 24) | (u32(g * 255) << 16) | (u32(b * 255) << 8); }
};

struct ColorRGBA {
    f32 r{};
    f32 g{};
    f32 b{};
    f32 a{};

    explicit operator glm::vec4() const { return glm::vec4{ r, g, b, a }; }
    u32 ToUint() const { return (u32(r * 255) << 24) | (u32(g * 255) << 16) | (u32(b * 255) << 8) | (u32(a * 255) << 0); }
};

ColorRGB FromHSV(f32 h, f32 s, f32 v);
ColorRGBA FromHSV(f32 h, f32 s, f32 v, f32 a);

} // namespace ugine
