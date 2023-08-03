#ifdef _WIN32

#include "Win32Window.h"

#include <ugine/File.h>
#include <ugine/engine/system/Event.h>

#include <iostream>

#include <windowsx.h>

#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ugine::win32 {

InputKeyCode ToKeyCode(WPARAM key) {
    // TODO
    switch (key) {
    case VK_ESCAPE: return InputKeyCode::Key_Escape;
    case VK_TAB: return InputKeyCode::Key_Tab;
    case VK_SHIFT: return InputKeyCode::Key_Shift;
    case VK_F1: return InputKeyCode::Key_F1;
    case VK_F2: return InputKeyCode::Key_F2;
    case VK_F3: return InputKeyCode::Key_F3;
    case VK_F4: return InputKeyCode::Key_F4;
    case VK_F5: return InputKeyCode::Key_F5;
    case VK_F6: return InputKeyCode::Key_F6;
    case VK_F7: return InputKeyCode::Key_F7;
    case VK_F8: return InputKeyCode::Key_F8;
    case VK_F9: return InputKeyCode::Key_F9;
    case VK_F10: return InputKeyCode::Key_F10;
    case VK_F11: return InputKeyCode::Key_F11;
    case VK_F12: return InputKeyCode::Key_F12;

    case '0': return InputKeyCode::Key_0;
    case '1': return InputKeyCode::Key_1;
    case '2': return InputKeyCode::Key_2;
    case '3': return InputKeyCode::Key_3;
    case '4': return InputKeyCode::Key_4;
    case '5': return InputKeyCode::Key_5;
    case '6': return InputKeyCode::Key_6;
    case '7': return InputKeyCode::Key_7;
    case '8': return InputKeyCode::Key_8;
    case '9': return InputKeyCode::Key_9;

    case 'A': return InputKeyCode::Key_A;
    case 'B': return InputKeyCode::Key_B;
    case 'C': return InputKeyCode::Key_C;
    case 'D': return InputKeyCode::Key_D;
    case 'E': return InputKeyCode::Key_E;
    case 'F': return InputKeyCode::Key_F;
    case 'G': return InputKeyCode::Key_G;
    case 'H': return InputKeyCode::Key_H;
    case 'I': return InputKeyCode::Key_I;
    case 'J': return InputKeyCode::Key_J;
    case 'K': return InputKeyCode::Key_K;
    case 'L': return InputKeyCode::Key_L;
    case 'M': return InputKeyCode::Key_M;
    case 'N': return InputKeyCode::Key_N;
    case 'O': return InputKeyCode::Key_O;
    case 'P': return InputKeyCode::Key_P;
    case 'Q': return InputKeyCode::Key_Q;
    case 'R': return InputKeyCode::Key_R;
    case 'S': return InputKeyCode::Key_S;
    case 'T': return InputKeyCode::Key_T;
    case 'U': return InputKeyCode::Key_U;
    case 'V': return InputKeyCode::Key_V;
    case 'W': return InputKeyCode::Key_W;
    case 'X': return InputKeyCode::Key_X;
    case 'Y': return InputKeyCode::Key_Y;
    case 'Z': return InputKeyCode::Key_Z;

    default: return InputKeyCode::Unknown;
    }
}

InputEvent::MouseButton ButtonToMouseButton(int button) {
    switch (button) {
    case 0: return InputEvent::MouseButton::Left;
    case 1: return InputEvent::MouseButton::Right;
    case 2: return InputEvent::MouseButton::Middle;
    default: return InputEvent::MouseButton::None;
    }
}

Win32Window::Win32Window() {
}

Win32Window::~Win32Window() {
    Close();
}

bool Win32Window::Init(
    HINSTANCE hInstance, const std::wstring& title, const std::wstring& name, LPCWSTR iconName, int width, int height, bool resizable, bool imgui) {
    imgui_ = imgui;

    //ImGui_ImplWin32_EnableDpiAwareness();

    width_ = width;
    height_ = height;

    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, iconName);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = title.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, iconName);
    if (!RegisterClassEx(&wcex)) {
        return false;
    }

    // Create window
    inst_ = hInstance;
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd_ = CreateWindow(title.c_str(), name.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | (resizable ? WS_SIZEBOX : 0), CW_USEDEFAULT,
        CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, this);

    if (!hwnd_) {
        return false;
    }

    SetWindowLongPtrA(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    if (imgui_) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        ImGui_ImplWin32_Init(hwnd_);
    }

    return true;
}

void Win32Window::Show() {
    Show(SW_SHOW);
}

void Win32Window::Show(int nCmdShow) {
    ShowWindow(hwnd_, nCmdShow);
}

void Win32Window::Close() {
    if (hwnd_) {
        if (imgui_) {
            ImGui_ImplWin32_Shutdown();
        }

        CloseWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Win32Window::SetTitle(const std::string_view& title) {
    SetWindowTextA(hwnd_, title.data());
}

void Win32Window::NewFrame() {
    if (imgui_) {
        ImGui_ImplWin32_NewFrame();
    }
}

std::pair<int, int> Win32Window::GetSize() const {
    RECT rc;

    GetClientRect(hwnd_, &rc);
    return { rc.right - rc.left, rc.bottom - rc.top };
}

bool Win32Window::Closed() const {
    return false;
}

void Win32Window::SetMouseCapture(bool capture) {
    captureMouse_ = capture;
    if (capture) {
        SetCursor(NULL);
    } else {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;

    auto win{ reinterpret_cast<Win32Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)) };
    if (win && win->imgui_ && !win->captureMouse_) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
            return true;
        }
    }

    switch (message) {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY: PostQuitMessage(0); break;

    case WM_MOUSEWHEEL:
        if (win) {
            const auto x{ GET_X_LPARAM(lParam) };
            const auto y{ GET_Y_LPARAM(lParam) };
            const auto zDelta{ GET_WHEEL_DELTA_WPARAM(wParam) };

            win->HandleInputEvent(InputEvent {
                .type = InputEvent::Type::MouseWheel,
                .mouseWheel = {
                    .x = x,
                    .y = y,
                    .delta = zDelta,
                },
            });
        }
        break;

    case WM_MOUSEMOVE:
        if (win) {
            const auto x{ GET_X_LPARAM(lParam) };
            const auto y{ GET_Y_LPARAM(lParam) };

            win->HandleInputEvent(InputEvent{
                .type = InputEvent::Type::MouseMove,
                .mouseMove = { .x = x, .y = y },
            });
        }
        break;

    case WM_LBUTTONDOWN:
        if (win) {
            const auto x{ GET_X_LPARAM(lParam) };
            const auto y{ GET_Y_LPARAM(lParam) };

            win->HandleInputEvent(InputEvent {
                .type = InputEvent::Type::MouseDown,
                .mouseDown = {
                    .x = x,
                    .y = y,
                    .button = InputEvent::MouseButton::Left,
                },
            });
        }
        break;

    case WM_LBUTTONUP:
        if (win) {
            const auto x{ GET_X_LPARAM(lParam) };
            const auto y{ GET_Y_LPARAM(lParam) };

            win->HandleInputEvent(InputEvent{
            .type = InputEvent::Type::MouseUp,
            .mouseUp = { .x = x, .y = y, .button = InputEvent::MouseButton::Left, },
            });
        }
        break;

    case WM_RBUTTONDOWN:
        if (win) {
            const auto x{ GET_X_LPARAM(lParam) };
            const auto y{ GET_Y_LPARAM(lParam) };

            win->HandleInputEvent(InputEvent {
                .type = InputEvent::Type::MouseDown,
                .mouseDown = {
                    .x = x,
                    .y = y,
                    .button = InputEvent::MouseButton::Right,
                },
            });
        }
        break;

    case WM_RBUTTONUP:
        if (win) {
            const auto x{ GET_X_LPARAM(lParam) };
            const auto y{ GET_Y_LPARAM(lParam) };

            win->HandleInputEvent(InputEvent{
            .type = InputEvent::Type::MouseUp,
            .mouseUp = { .x = x, .y = y, .button = InputEvent::MouseButton::Right, },
        });
        }
        break;

    case WM_KEYUP:
        if (win) {
            win->HandleInputEvent(InputEvent{
                .type = InputEvent::Type::KeyUp,
                .keyUp = { .key = ToKeyCode(wParam) },
            });
        }
        break;

    case WM_KEYDOWN:
        if (win) {
            win->HandleInputEvent(InputEvent{
                .type = InputEvent::Type::KeyDown,
                .keyDown = { .key = ToKeyCode(wParam) },
            });
        }
        break;

    case WM_SIZE:
        if (win) {
            if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED || wParam == SIZE_MINIMIZED) {
                if (wParam == SIZE_MINIMIZED) {
                    win->visible_ = false;
                    win->HandleSystemEvent(SystemEvent{
                        .type = SystemEvent::Type::WindowShow,
                        .show = { .show = false },
                    });
                } else {
                    win->visible_ = true;
                    win->HandleSystemEvent(SystemEvent{
                        .type = SystemEvent::Type::WindowResized, .resize={ .width = LOWORD(lParam), .height = HIWORD(lParam),
                        },
                    });
                }
            }
        }
        break;

    case WM_ENTERSIZEMOVE:
        if (win) {
            win->SetResizing(true);
        }
        break;

    case WM_EXITSIZEMOVE:
        if (win) {
            win->SetResizing(false);
        }
        break;

    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return FALSE;
}
void Win32Window::HandleResize(unsigned int width, unsigned int height) {
    newWidth_ = width;
    height_ = height;

    if (!resizing_) {
        UpdateSize();
    }
}

bool Win32Window::HandleMouseCapture() {
    // TODO:
    //POINT point{};
    //GetCursorPos(&point);
    //if (point.x != lastMouseX_ || point.y != lastMouseY_) {
    //    lastMouseX_ = point.x;
    //    lastMouseY_ = point.y;

    //    HandleInputEvent(
    //        InputEvent{ 
    //            .type = InputEvent::Type::MouseMove, 
    //            .mouseMove = {
    //                .x = point.x,
    //                .y = point.y,
    //            },
    //        });

    //    return true;
    //}

    return false;
}

void Win32Window::HandleInputEvent(const InputEvent& event) {
    if (imgui_) {
        // TODO: ImGui capture input?

        switch (event.type) {
            using enum InputEvent::Type;

        case KeyDown:
        case KeyUp: break;
        case MouseDown:
        case MouseUp:
        case MouseMove:
        case MouseWheel: break;
        }
    }

    HandleSystemEvent(SystemEvent{ .type = SystemEvent::Type::Input, .input = event });
}

void Win32Window::SetResizing(bool resizing) {
    resizing_ = resizing;

    if (!resizing_) {
        UpdateSize();
    }
}

void Win32Window::UpdateSize() {
    //if (newWidth_ != width_ || height_ != height_) {
    //    width_ = newWidth_;
    //    height_ = height_;

    //    if (systemHandler_) {
    //        systemHandler_(SystemEvent{
    //            .type = SystemEvent::Type::WindowResized,
    //            .resize = { .width = width_, .height = height_, },
    //        });
    //    }
    //}
}

} // namespace ugine::win32

#endif // _WIN32
