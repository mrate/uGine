#include "InputState.h"
#include "Input.h"

#include <ugine/engine/engine/Engine.h>

namespace ugine {

InputState::InputState(const Engine& engine)
    : engine_{ engine }
    , controllers_{ engine.GetAllocator() } {
    SetupDefaultMapping();
}

InputState::~InputState() {
}

void InputState::SetupDefaultMapping() {
    SetMapping(INPUT_MOUSE_DRAG_X, ACTION_LEFT_ANALOG_X);
    SetMapping(INPUT_MOUSE_DRAG_Y, ACTION_LEFT_ANALOG_Y);

    SetMapping(INPUT_MOUSE_X, ACTION_POINTER_X);
    SetMapping(INPUT_MOUSE_Y, ACTION_POINTER_Y);
    SetMapping(INPUT_MOUSE_Z, ACTION_POINTER_Z);

    SetMapping(INPUT_MOUSE_LBUTTON, ACTION_POINTER_B1);
    SetMapping(INPUT_MOUSE_RBUTTON, ACTION_POINTER_B2);
    SetMapping(INPUT_MOUSE_MBUTTON, ACTION_POINTER_B3);

    SetMapping(INPUT_LEFT_ANALOG_X, ACTION_LEFT_ANALOG_X);
    SetMapping(INPUT_LEFT_ANALOG_Y, ACTION_LEFT_ANALOG_Y);
    SetMapping(INPUT_LEFT_ANALOG_Z, ACTION_LEFT_ANALOG_Z);

    SetMapping(INPUT_RIGHT_ANALOG_X, ACTION_RIGHT_ANALOG_X);
    SetMapping(INPUT_RIGHT_ANALOG_Y, ACTION_RIGHT_ANALOG_Y);
    SetMapping(INPUT_RIGHT_ANALOG_Z, ACTION_RIGHT_ANALOG_Z);

    SetMapping(INPUT_CONTROLLER_B1, ACTION_CONTROLLER_B1);
    SetMapping(INPUT_CONTROLLER_B2, ACTION_CONTROLLER_B2);
    SetMapping(INPUT_CONTROLLER_B3, ACTION_CONTROLLER_B3);
    SetMapping(INPUT_CONTROLLER_B4, ACTION_CONTROLLER_B4);
}

void InputState::SetInput(InputIdRef input, f32 value) {
    auto it{ actionMappings_.find(input) };
    if (it != actionMappings_.end()) {
        for (const auto& mapping : it->second) {
            actionValues_[mapping] = value;
        }
    }
}

void InputState::AddInput(InputIdRef input, f32 value) {
    auto it{ actionMappings_.find(input) };
    if (it != actionMappings_.end()) {
        for (const auto& mapping : it->second) {
            actionValues_[mapping] += value;
        }
    }
}

void InputState::SetMapping(InputIdRef input, ActionIdRef action) {
    actionMappings_[input].insert(action);
}

void InputState::RemoveMapping(InputIdRef input, ActionIdRef action) {
    actionMappings_[input].erase(action);
}

f32 InputState::GetAction(ActionIdRef action) {
    auto it{ actionValues_.find(action) };
    return it != actionValues_.end() ? it->second : 0.0f;
}

void InputState::SetKeyDown(InputKeyCode key) {
    keys_.insert(std::make_pair(key, engine_.FrameNumber()));
}

void InputState::SetKeyUp(InputKeyCode key) {
    keys_.erase(key);
}

bool InputState::KeyDown(InputKeyCode key) {
    return keys_.find(key) != keys_.end();
}

bool InputState::KeyPressed(InputKeyCode key) {
    auto it{ keys_.find(key) };
    return it != keys_.end() && it->second == engine_.FrameNumber();
}

void InputState::UpdateControllers() {
    for (auto& ctrl : controllers_) {
        ctrl->ReadState();

        if (!ctrl->IsConnected()) {
            continue;
        }

        AddInput(INPUT_LEFT_ANALOG_X, ctrl->LeftAnalogX());
        AddInput(INPUT_LEFT_ANALOG_Y, ctrl->LeftAnalogY());

        AddInput(INPUT_RIGHT_ANALOG_X, ctrl->RightAnalogX());
        AddInput(INPUT_RIGHT_ANALOG_Y, ctrl->RightAnalogY());

        AddInput(INPUT_CONTROLLER_B1, ctrl->ButtonA());
        AddInput(INPUT_CONTROLLER_B2, ctrl->ButtonB());
        AddInput(INPUT_CONTROLLER_B3, ctrl->ButtonX());
        AddInput(INPUT_CONTROLLER_B4, ctrl->ButtonY());
    }
}

void InputState::RegisterController(UniquePtr<InputController> controller) {
    controllers_.PushBack(std::move(controller));
}

//void InputState::UnRegisterController(Controller* controller) {
//    auto it{ std::remove(controllers_.begin(), controllers_.end(), controller) };
//    controllers_.erase(it, controllers_.end());
//}

} // namespace ugine
