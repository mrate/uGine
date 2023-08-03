#pragma once

#include "Input.h"
#include "InputController.h"

#include <ugine/Memory.h>
#include <ugine/Vector.h>

#include <map>
#include <set>

namespace ugine {

class Engine;

class InputState {
public:
    explicit InputState(const Engine& engine);
    ~InputState();

    InputState(InputState&& other) = default;
    InputState& operator=(InputState&& other) = default;

    void SetInput(InputIdRef input, f32 value);
    void AddInput(InputIdRef input, f32 value);

    void SetMapping(InputIdRef input, ActionIdRef action);
    void RemoveMapping(InputIdRef input, ActionIdRef action);

    f32 GetAction(ActionIdRef action);

    void SetKeyDown(InputKeyCode key);
    void SetKeyUp(InputKeyCode key);
    bool KeyDown(InputKeyCode key);
    bool KeyPressed(InputKeyCode key);

    void UpdateControllers();
    void RegisterController(UniquePtr<InputController> controller);

private:
    void SetupDefaultMapping();

    const Engine& engine_;

    std::map<ActionId, f32> actionValues_;
    std::map<InputId, std::set<ActionId>> actionMappings_;

    // Key -> frame number mapping.
    std::map<InputKeyCode, u64> keys_;

    Vector<UniquePtr<InputController>> controllers_;
};

} // namespace ugine
