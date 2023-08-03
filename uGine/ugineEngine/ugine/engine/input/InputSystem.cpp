#include "InputSystem.h"
#include "InputState.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/system/Platform.h>

namespace ugine {

InputId ButtonToInput(InputEvent::MouseButton button) {
    switch (button) {
        using enum InputEvent::MouseButton;

    case Left: return INPUT_MOUSE_LBUTTON;
    case Right: return INPUT_MOUSE_RBUTTON;
    default: return INPUT_MOUSE_MBUTTON;
    }
}

InputSystem::InputSystem(Engine& engine)
    : System{ engine }
    , input_{ engine.AttachState<InputState>(engine) } {
    auto controllers{ engine.GetPlatform().CreateControllers(engine.GetAllocator()) };

    for (auto& controller : controllers) {
        input_.RegisterController(std::move(controller));
    }
}

InputSystem::~InputSystem() {
    GetEngine().DeleteState<InputState>();
}

void InputSystem::EarlyUpdate() {
    // Reset input.
    input_.SetInput(INPUT_MOUSE_DRAG_X, 0.0f);
    input_.SetInput(INPUT_MOUSE_DRAG_Y, 0.0f);
    input_.SetInput(INPUT_MOUSE_Z, 0.0f);

    // Update input.
    input_.UpdateControllers();

    // TODO:
    //keys_.shift[0] = GetAsyncKeyState(VK_LSHIFT) != 0;
    //keys_.shift[1] = GetAsyncKeyState(VK_RSHIFT) != 0;
    //keys_.ctrl[0] = GetAsyncKeyState(VK_LCONTROL) != 0;
    //keys_.ctrl[1] = GetAsyncKeyState(VK_RCONTROL) != 0;
    //keys_.alt[0] = GetAsyncKeyState(VK_LMENU) != 0;
    //keys_.alt[1] = GetAsyncKeyState(VK_RMENU) != 0;
}

void InputSystem::HandleInputEvent(const InputEvent& event) {
    switch (event.type) {
        using enum InputEvent::Type;

    case KeyDown:
        //if (!gui_ || !gui_->CaptureKeyboard()) {
        input_.SetKeyDown(event.keyDown.key);
        //}
        break;
    case KeyUp:
        //if (!gui_ || !gui_->CaptureKeyboard()) {
        input_.SetKeyUp(event.keyUp.key);
        //}
        break;
    case MouseWheel:
        //if (!gui_ || !gui_->CaptureMouse()) {
        input_.SetInput(INPUT_MOUSE_Z, static_cast<f32>(event.mouseWheel.delta));
        input_.SetInput(INPUT_MOUSE_X, static_cast<f32>(event.mouseWheel.x));
        input_.SetInput(INPUT_MOUSE_Y, static_cast<f32>(event.mouseWheel.y));
        //}
        break;
    case MouseMove:
        //if (!gui_ || !gui_->CaptureMouse()) {
        input_.SetInput(INPUT_MOUSE_X, static_cast<f32>(event.mouseMove.x));
        input_.SetInput(INPUT_MOUSE_Y, static_cast<f32>(event.mouseMove.y));

        if (mouse_.rightDown) {
            input_.AddInput(INPUT_MOUSE_DRAG_X, static_cast<f32>(event.mouseMove.x - mouse_.lastPosX));
            input_.AddInput(INPUT_MOUSE_DRAG_Y, static_cast<f32>(event.mouseMove.y - mouse_.lastPosY));

            mouse_.lastPosX = event.mouseMove.x;
            mouse_.lastPosY = event.mouseMove.y;
        }
        //}
        break;
    case MouseUp:
        //if (!gui_ || !gui_->CaptureMouse()) {
        input_.SetInput(ButtonToInput(event.mouseUp.button), 0.0f);
        input_.SetInput(INPUT_MOUSE_X, static_cast<f32>(event.mouseUp.x));
        input_.SetInput(INPUT_MOUSE_Y, static_cast<f32>(event.mouseUp.y));
        //}
        break;
    case MouseDown:
        //if (!gui_ || !gui_->CaptureMouse()) {
        input_.SetInput(ButtonToInput(event.mouseUp.button), 1.0f);
        input_.SetInput(INPUT_MOUSE_X, static_cast<f32>(event.mouseUp.x));
        input_.SetInput(INPUT_MOUSE_Y, static_cast<f32>(event.mouseUp.y));
        //}
        break;
    }
}

void InputSystem::LateUpdate() {
}

bool InputSystem::HandleSystemEvent(const SystemEvent& event) {
    switch (event.type) {
        using enum SystemEvent::Type;

    case Input: HandleInputEvent(event.input); break;
    }

    return false;
}

} // namespace ugine