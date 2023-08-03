#include "Color.h"

#include <cmath>

namespace ugine {

ColorRGB FromHSV(f32 h, f32 s, f32 v) {
    //const auto c{ (1 - std::abs(2 * l - 1)) * s };
    const auto c{ v * s };
    const auto x{ c * (1 - std::abs(std::fmod(h / 60.0f, 2.0f) - 1)) };
    //const auto m{ l - c / 2 };
    const auto m{ v - c };

    ColorRGB rgb{};
    if (h < 60) {
        rgb = ColorRGB{ c, x, 0 };
    } else if (h < 120) {
        rgb = ColorRGB{ x, c, 0 };
    } else if (h < 180) {
        rgb = ColorRGB{ 0, c, x };
    } else if (h < 240) {
        rgb = ColorRGB{ 0, x, c };
    } else if (h < 300) {
        rgb = ColorRGB{ x, 0, c };
    } else {
        rgb = ColorRGB{ c, 0, x };
    }

    return ColorRGB{ rgb.r + m, rgb.g + m, rgb.b + m };
}

ColorRGBA FromHSV(f32 h, f32 s, f32 v, f32 a) {
    const auto rgb{ FromHSV(h, s, v) };
    return ColorRGBA{ rgb.r, rgb.g, rgb.b, a };
}

} // namespace ugine