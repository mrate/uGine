#pragma once

#include "Event.h"

#include <gfxapi/Device.h>

#include <ugine/Memory.h>
#include <ugine/Vector.h>
#include <ugine/engine/input/InputController.h>

#include <optional>
#include <string_view>

namespace ugine {

class Window;
struct EngineParams;

class Platform {
public:
    static UniquePtr<Platform> Create(const EngineParams& params, IAllocator& allocator = IAllocator::Default());

    virtual ~Platform() = default;

    virtual void Update() = 0;
    virtual void UpdateTitle(std::string_view title) = 0;
    virtual std::optional<SystemEvent> PollSystemEvent() = 0;
    virtual void* GetNativeHandle() const = 0;

    virtual void SetMouseCapture(bool capture) = 0;

    // Graphics
    virtual void FillDeviceCreateInfo(gfxapi::DeviceCreateInfo& info) const = 0;

    // Input
    virtual Vector<UniquePtr<InputController>> CreateControllers(IAllocator& allocator) const = 0;
};

} // namespace ugine