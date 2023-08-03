#pragma once

#include <ugine/engine/engine/System.h>
#include <ugine/engine/input/InputState.h>

namespace ugine {

class Platform;

class InputSystem final : public System {
public:
    explicit InputSystem(Engine& engine);
    ~InputSystem();

    void EarlyUpdate();
    void LateUpdate();

    bool HandleSystemEvent(const SystemEvent& event) override;

private:
    void HandleInputEvent(const InputEvent& event);

    bool IsShiftPressed(bool left) const { return keys_.shift[left ? 0 : 1]; }

    bool IsCtrlPressed(bool left) const { return keys_.ctrl[left ? 0 : 1]; }

    bool IsAltPressed(bool left) const { return keys_.alt[left ? 0 : 1]; }

    struct Mouse {
        int lastPosX{};
        int lastPosY{};
        bool leftDown{};
        bool rightDown{};
    } mouse_;

    struct Keys {
        bool shift[2];
        bool ctrl[2];
        bool alt[2];
    } keys_;

    InputState& input_;
};

} // namespace ugine