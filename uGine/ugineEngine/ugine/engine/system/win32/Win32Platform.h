#pragma once

#ifdef _WIN32
#include <ugine/engine/engine/Params.h>

#include "../Platform.h"
#include "Win32Window.h"

#include <Windows.h>

namespace ugine::win32 {

class Win32Platform final : public Platform {
public:
    Win32Platform(const EngineParams& params);
    ~Win32Platform();

    void Update() override;
    void UpdateTitle(std::string_view title) override;
    void FillDeviceCreateInfo(ugine::gfxapi::DeviceCreateInfo& info) const override;
    void* GetNativeHandle() const override;

    void SetMouseCapture(bool capture) override;

    Vector<UniquePtr<InputController>> CreateControllers(IAllocator& allocator) const override;

    std::optional<SystemEvent> PollSystemEvent() override;

private:
    bool imgui_{};
    Win32Window window_;
};

} // namespace ugine::win32

#endif // _WIN32
