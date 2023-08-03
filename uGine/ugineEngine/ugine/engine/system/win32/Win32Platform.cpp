#ifdef _WIN32

#include "Win32Platform.h"
#include "DirectInput.h"
#include "Win32Window.h"

#include <ugine/Error.h>
#include <ugine/String.h>
#include <ugine/StringUtils.h>

namespace ugine {

UniquePtr<Platform> Platform::Create(const EngineParams& params, IAllocator& allocator) {
    return MakeUnique<win32::Win32Platform>(allocator, params);
}
} // namespace ugine

namespace ugine::win32 {

Win32Platform::Win32Platform(const EngineParams& params) {
    const auto wTitle{ ToWString(
        std::format("{} [v{}.{}.{}]", params.appName.Data(), params.appVersion.major, params.appVersion.minor, params.appVersion.file)) };

    // TODO: Fullscreen.
    auto hInstance{ GetModuleHandle(nullptr) };
    if (!window_.Init(hInstance, wTitle, wTitle, nullptr, params.width, params.height, true, params.imgui)) {
        UGINE_THROW(Error, "Failed to create window.");
    }

    window_.Show();
}

win32::Win32Platform::~Win32Platform() {
}

void win32::Win32Platform::Update() {
    window_.NewFrame();
}

void Win32Platform::UpdateTitle(std::string_view title) {
    window_.SetTitle(title);
}

void Win32Platform::FillDeviceCreateInfo(gfxapi::DeviceCreateInfo& info) const {
    info.win32.hInstance = window_.Instance();
    info.win32.hWnd = window_.Handle();
}

void* win32::Win32Platform::GetNativeHandle() const {
    return window_.Handle();
}

void Win32Platform::SetMouseCapture(bool capture) {
    window_.SetMouseCapture(capture);
}

Vector<UniquePtr<InputController>> Win32Platform::CreateControllers(IAllocator& allocator) const {
    Vector<UniquePtr<InputController>> controllers{ allocator };

    auto directInput{ std::make_shared<DirectInput>() };
    for (auto i : directInput->AttachedControllers(allocator)) {
        controllers.PushBack(directInput->CreateController(allocator, i.id));
    }

    return controllers;
}

std::optional<SystemEvent> Win32Platform::PollSystemEvent() {
    MSG msg = { 0 };

    if (window_.Update()) {
        return window_.PopEvent();
    }

    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);

        if (msg.message == WM_QUIT) {
            return SystemEvent{ .type = SystemEvent::Type::Quit };
        }

        DispatchMessage(&msg);

        return window_.PopEvent();
    }

    // TODO:
    return {};
}

} // namespace ugine::win32

#endif // _WIN32
