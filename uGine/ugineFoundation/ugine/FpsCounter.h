#pragma once

#include <chrono>

namespace ugine {

//template <typename Clock = std::chrono::high_resolution_clock>
class FpsCounter {
public:
    FpsCounter() {
        lastTime_ = Clock::now();
        frames_ = 0;
    }

    bool Fps(f32& fps) {
        auto time{ Clock::now() };
        ++frames_;
        if (time - lastTime_ >= std::chrono::seconds(1)) {
            fps = fps_ = frames_ / (static_cast<f32>(std::chrono::duration_cast<std::chrono::milliseconds>(time - lastTime_).count()) / 1000.0f);
            frames_ = 0;
            lastTime_ = time;

            return true;
        }

        fps = fps_;
        return false;
    }

private:
    using Clock = std::chrono::high_resolution_clock;

    Clock::time_point lastTime_;
    f32 fps_{};
    int frames_{};
};

} // namespace ugine