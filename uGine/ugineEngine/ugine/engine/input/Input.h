#pragma once

#include <ugine/String.h>

namespace ugine {

// TODO: Hashed strings.

using InputId = StringID;
using ActionId = StringID;

using InputIdRef = const InputId&;
using ActionIdRef = const ActionId&;

const InputId INPUT_MOUSE_X{ "MOUSE_X" };
const InputId INPUT_MOUSE_Y{ "MOUSE_Y" };
const InputId INPUT_MOUSE_Z{ "MOUSE_Z" };
const InputId INPUT_MOUSE_LBUTTON{ "MOUSE_LBUTTON" };
const InputId INPUT_MOUSE_RBUTTON{ "MOUSE_RBUTTON" };
const InputId INPUT_MOUSE_MBUTTON{ "MOUSE_MBUTTON" };

const InputId INPUT_MOUSE_DRAG_X{ "MOUSE_DRAG_X" };
const InputId INPUT_MOUSE_DRAG_Y{ "MOUSE_DRAG_Y" };

const InputId INPUT_LEFT_ANALOG_X{ "LEFT_ANALOG_X" };
const InputId INPUT_LEFT_ANALOG_Y{ "LEFT_ANALOG_Y" };
const InputId INPUT_LEFT_ANALOG_Z{ "LEFT_ANALOG_Z" };

const InputId INPUT_RIGHT_ANALOG_X{ "RIGHT_ANALOG_X" };
const InputId INPUT_RIGHT_ANALOG_Y{ "RIGHT_ANALOG_Y" };
const InputId INPUT_RIGHT_ANALOG_Z{ "RIGHT_ANALOG_Z" };

const InputId INPUT_CONTROLLER_B1{ "CONTROLLER_B1" };
const InputId INPUT_CONTROLLER_B2{ "CONTROLLER_B2" };
const InputId INPUT_CONTROLLER_B3{ "CONTROLLER_B3" };
const InputId INPUT_CONTROLLER_B4{ "CONTROLLER_B4" };

const ActionId ACTION_LEFT_ANALOG_X{ "LEFT_ANALOG_X" };
const ActionId ACTION_LEFT_ANALOG_Y{ "LEFT_ANALOG_Y" };
const ActionId ACTION_LEFT_ANALOG_Z{ "LEFT_ANALOG_Z" };

const ActionId ACTION_RIGHT_ANALOG_X{ "RIGHT_ANALOG_X" };
const ActionId ACTION_RIGHT_ANALOG_Y{ "RIGHT_ANALOG_Y" };
const ActionId ACTION_RIGHT_ANALOG_Z{ "RIGHT_ANALOG_Z" };

const ActionId ACTION_CONTROLLER_B1{ "CONTROLLER_B1" };
const ActionId ACTION_CONTROLLER_B2{ "CONTROLLER_B2" };
const ActionId ACTION_CONTROLLER_B3{ "CONTROLLER_B3" };
const ActionId ACTION_CONTROLLER_B4{ "CONTROLLER_B4" };

const ActionId ACTION_POINTER_X{ "POINTER_X" };
const ActionId ACTION_POINTER_Y{ "POINTER_Y" };
const ActionId ACTION_POINTER_Z{ "POINTER_Z" };
const ActionId ACTION_POINTER_B1{ "POINTER_B1" };
const ActionId ACTION_POINTER_B2{ "POINTER_B2" };
const ActionId ACTION_POINTER_B3{ "POINTER_B3" };

enum class InputKeyCode : u16 {
    Unknown = 0,
    Key_Escape,
    Key_Tab,

    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    Key_Shift,

    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
};

} // namespace ugine
