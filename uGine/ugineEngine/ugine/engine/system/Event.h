#pragma once

#include <ugine/Ugine.h>
#include <ugine/engine/input/Input.h>

#include <functional>
#include <stdint.h>

namespace ugine {

struct InputEvent {
    enum class Type {
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        MouseWheel,
    };

    enum class MouseButton {
        None,
        Left,
        Right,
        Middle,
    };

    Type type{};

    union {
        struct {
            InputKeyCode key;
        } keyDown;
        struct {
            InputKeyCode key;
        } keyUp;
        struct {
            int x;
            int y;
            MouseButton button;
        } mouseDown;
        struct {
            int x;
            int y;
            MouseButton button;
        } mouseUp;
        struct {
            int x;
            int y;
        } mouseMove;
        struct {
            int x;
            int y;
            int delta;
        } mouseWheel;
    };
};

struct SystemEvent {
    enum class Type {
        WindowResized,
        WindowShow,
        Input,
        Quit,
    };

    Type type{};

    union {
        struct {
            u32 width;
            u32 height;
        } resize;

        struct {
            bool show;
        } show;
    };
    InputEvent input;
};

} // namespace ugine
