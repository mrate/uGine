#pragma once

#include <chrono>
#include <cmath>
#include <functional>

#include <glm/glm.hpp>

#include "Math.h"

namespace ugine {

// https://easings.net/

constexpr f32 EasingLinear(f32 t) {
    return t;
}

constexpr f32 EasingCubic(f32 x) {
    return x < 0.5 ? 4 * x * x * x : 1 - std::powf(-2 * x + 2, 3) / 2;
}

constexpr f32 EasingElastic(f32 x) {
    const auto c5{ (2 * glm::pi<f32>()) / 4.5f };

    return x == 0  ? 0
        : x == 1   ? 1
        : x < 0.5f ? -(std::powf(2, 20 * x - 10) * sinf((20 * x - 11.125f) * c5)) / 2
                   : (std::powf(2, -20 * x + 10) * std::sinf((20 * x - 11.125f) * c5)) / 2 + 1;
}

template <typename Value> struct AnimatedValue {
public:
    using Clock = std::chrono::high_resolution_clock;

    // TODO: std::function
    using EasingFunc = std::function<f32(f32)>;

    AnimatedValue() = default;

    explicit AnimatedValue(Value value)
        : m_from{ value }
        , m_to{ value } {}

    AnimatedValue(Value from, Value to, std::chrono::duration<f32> duration, Clock::time_point start = Clock::now(), bool repeat = false,
        EasingFunc easingFunc = EasingLinear)
        : m_from{ from }
        , m_to{ to }
        , m_start{ start }
        , m_duration{ duration }
        , m_repeat{ repeat }
        , m_easingFunc{ easingFunc } {}

    Value GetValue(Clock::time_point time) const {
        const auto t{ m_easingFunc(std::clamp(GetPercentageTime(time), 0.0f, 1.0f)) };
        return (1 - t) * m_from + t * m_to;
    }

    bool IsFinished(Clock::time_point time) const { return time > (m_start + m_duration); }

private:
    f32 GetPercentageTime(Clock::time_point time) const {
        const auto elapsed{ std::chrono::duration<f32>(time - m_start).count() };
        if (m_repeat) {
            const auto elepsedPercent{ elapsed / m_duration.count() };
            return elepsedPercent - std::floorf(elepsedPercent);
        } else {
            return elapsed / m_duration.count();
        }
    }

    Clock::time_point m_start{};
    std::chrono::duration<f32> m_duration{};

    Value m_from{};
    Value m_to{};

    EasingFunc m_easingFunc{ EasingLinear };

    bool m_repeat{};
};

} // namespace ugine