#include "CameraController.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/input/InputState.h>
#include <ugine/engine/math/Math.h>
#include <ugine/engine/world/Transformation.h>

#include <glm/glm.hpp>

namespace ugine {

void CameraController::Init(const Transformation& transformation) {
    const auto euler{ glm::eulerAngles(transformation.rotation) };
    pitch_ = euler.x;
    yaw_ = euler.y;
}

void CameraController::Update(Engine& engine, Transformation& transformation) {
    const f32 time{ std::min(0.02f, f32(engine.FrameSeconds())) };

    auto state{ engine.GetState<InputState>() };
    UGINE_ASSERT(state);

    const f32 speed{ 4.0f * time * (state->KeyDown(InputKeyCode::Key_Shift) ? 2.0f : 1.0f) + std::abs(state->GetAction(ACTION_POINTER_Z) * 0.001f) };
    const f32 rotSpeed{ 0.25f * time };

    f32 analogX{ state->GetAction(ACTION_LEFT_ANALOG_X) };
    f32 analogY{ state->GetAction(ACTION_LEFT_ANALOG_Y) };

    bool forward{ state->KeyDown(InputKeyCode::Key_W) || state->GetAction(ACTION_POINTER_Z) > 0 || state->GetAction(ACTION_RIGHT_ANALOG_Y) < 0.0f };
    bool backward{ state->KeyDown(InputKeyCode::Key_S) || state->GetAction(ACTION_POINTER_Z) < 0 || state->GetAction(ACTION_RIGHT_ANALOG_Y) > 0.0f };
    bool left{ state->KeyDown(InputKeyCode::Key_A) || state->GetAction(ACTION_RIGHT_ANALOG_X) < 0.0f };
    bool right{ state->KeyDown(InputKeyCode::Key_D) || state->GetAction(ACTION_RIGHT_ANALOG_X) > 0.0f };
    bool up{ state->KeyDown(InputKeyCode::Key_E) };
    bool down{ state->KeyDown(InputKeyCode::Key_Q) };

    if (!(forward || backward || left || right || up || down || analogX != 0.0f || analogY != 0.0f)) {
        return;
    }

    yaw_ -= fmod(analogX * rotSpeed * glm::pi<f32>(), glm::pi<f32>() * 2.0f);
    pitch_ -= analogY * rotSpeed * glm::pi<f32>();
    pitch_ = std::max(std::min(pitch_, glm::pi<f32>() * 0.499f), -glm::pi<f32>() * 0.499f);

    const auto heading{ glm::fquat(glm::vec3{ 0.0f, yaw_, 0.0f }) };
    const auto rotation{ glm::fquat(glm::vec3{ pitch_, yaw_, 0.0f }) };

    glm::vec3 forwardOffset{ rotation * ugine::math::FORWARD };
    glm::vec3 sideOffset{ glm::cross(heading * ugine::math::FORWARD, ugine::math::UP) };
    glm::vec3 upOffset{ ugine::math::UP };

    if (left || right) {
        sideOffset *= (left ? -1 : 1) * speed;
    } else {
        sideOffset = glm::vec3(0.0f);
    }

    if (up || down) {
        upOffset *= (down ? -1 : 1) * speed;
    } else {
        upOffset = glm::vec3(0.0f);
    }

    if (forward || backward) {
        forwardOffset *= (forward ? 1 : -1) * speed;
    } else {
        forwardOffset = glm::vec3(0.0f);
    }

    transformation.position = transformation.position + forwardOffset + sideOffset + upOffset;
    transformation.rotation = rotation;
}

} // namespace ugine
