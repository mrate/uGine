#pragma once
#ifdef _WIN32

#include <ugine/engine/input/InputController.h>

#include <cmath>
#include <string>

namespace ugine::win32 {

class XControllerDevice : public InputController {
public:
    struct Info {
        std::wstring name;
        std::string id;
    };

    virtual ~XControllerDevice() {}

    f32 LeftAnalogX() const override { return m_leftAnalog.x; }
    f32 LeftAnalogY() const override { return m_leftAnalog.y; }
    bool LeftAnalogPressed() const override { return m_leftAnalog.pressed; }

    f32 RightAnalogX() const override { return m_rightAnalog.x; }
    f32 RightAnalogY() const override { return m_rightAnalog.y; }
    bool RightAnalogPressed() const override { return m_rightAnalog.pressed; }

    f32 ButtonA() const override { return m_buttons.a; }
    f32 ButtonB() const override { return m_buttons.b; }
    f32 ButtonX() const override { return m_buttons.x; }
    f32 ButtonY() const override { return m_buttons.y; }

protected:
    struct Analog {
        f32 x{};
        f32 y{};
        bool pressed{};
    };

    struct Buttons {
        bool a{};
        bool b{};
        bool x{};
        bool y{};
    };

    f32 DeadZoneNorm(f32 x, f32 val) const {
        if (x < 0) {
            x += val;
            x = fminf(0, fmaxf(x, -32000)); // clamp(x, -32000, 0);
        } else {
            x -= val;
            x = fminf(32000, fmaxf(x, 0)); // clamp(x, 0, 32000);
        }
        x *= (1 / 32000.0f);
        return x;
    }

    void SetState(const Analog& leftAnalog, const Analog& rightAnalog, const Buttons& buttons) {
        m_leftAnalog = leftAnalog;
        m_rightAnalog = rightAnalog;
        m_buttons = buttons;
    }

private:
    Analog m_leftAnalog{};
    Analog m_rightAnalog{};
    Buttons m_buttons{};
};

} // namespace ugine::win32

#endif
