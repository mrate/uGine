#pragma once

#include "Log.h"
#include "String.h"
#include "StringUtils.h"

#include <chrono>
#include <string_view>

constexpr auto COLOR_PROFILE_GRAPHICS{ 0xffff7f25 };
constexpr auto COLOR_PROFILE_ENGINE{ 0xff6060ff };
constexpr auto COLOR_PROFILE_FILESYSTEM{ 0xff60ff60 };

#ifdef UGINE_PROFILE_OPTICK

#include <optick.h>

#define PROFILE_EVENT() OPTICK_EVENT()
#define PROFILE_EVENT_N(NAME) OPTICK_EVENT(NAME)
#define PROFILE_TAG OPTICK_TAG

#define PROFILE_FRAME OPTICK_FRAME

#define PROFILE_THREAD(NAME) OPTICK_THREAD(NAME)
#define PROFILE_START_THREAD(NAME) OPTICK_START_THREAD(NAME)
#define PROFILE_STOP_THREAD() OPTICK_STOP_THREAD()

#define PROFILE_SHUTDOWN OPTICK_SHUTDOWN

#define PROFILE_GPU_INIT(DEVICE, PHYSDEVICE, QUEUE, QUEUEFAMILY) OPTICK_GPU_INIT_VULKAN(&DEVICE, &PHYSDEVICE, &QUEUE, &QUEUEFAMILY, 1, nullptr)
#define PROFILE_GPU_CONTEXT(CMDLIST) OPTICK_GPU_CONTEXT(ugine::gfx::GraphicsService::Device().CommandBuffer(CMDLIST))
#define PROFILE_GPU_EVENT(CMDLIST, NAME) OPTICK_GPU_EVENT(NAME)
#define PROFILE_GPU_COLLECT(...)
#define PROFILE_GPU_PRESENT(SWAPCHAIN) OPTICK_GPU_FLIP(SWAPCHAIN)

#elif UGINE_PROFILE_TRACY

#include <vulkan/vulkan.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

namespace ugine::profile {
extern tracy::VkCtx* TracyVkContext;
}

#define PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define PROFILE_FREE(ptr) TracyFree(ptr)

#define PROFILE_EVENT() ZoneScoped
#define PROFILE_EVENT_N(NAME) ZoneScopedN(NAME)
#define PROFILE_EVENT_NC(NAME, COLOR) ZoneScopedNC(NAME, COLOR)
#define PROFILE_TAG(...)

#define PROFILE_FRAME(...)

#define PROFILE_THREAD(NAME) ::tracy::SetThreadName(NAME)
#define PROFILE_START_THREAD(NAME) ::tracy::SetThreadName(NAME)
#define PROFILE_STOP_THREAD()

#define PROFILE_SHUTDOWN(...)

#define PROFILE_GPU_INIT(...)
#define PROFILE_GPU_CONTEXT(CMDLIST)
#define PROFILE_GPU_EVENT(CMDLIST, NAME) TracyVkZone(ugine::profile::TracyVkContext, VkCommandBuffer{ (CMDLIST).NativePtr() }, NAME)
#define PROFILE_GPU_COLLECT(CMDLIST) TracyVkCollect(ugine::profile::TracyVkContext, CMDLIST)
#define PROFILE_GPU_PRESENT(...) FrameMark

#define PROFILE_MESSAGE(msg) TracyMessageL(msg)
#define PROFILE_MESSAGE_DYN(msg, size) TracyMessage(msg, size)

#define PROFILE_PLOT(name, value) TracyPlot(name, value)
#define PROFILE_PLOT_NUMBER(name, step, color) TracyPlotConfig(name, ::tracy::PlotFormatType::Number, step, false, color)

#define PROFILE_LOCKABLE(var, name) TracyLockable(var, name)

#else // UGINE_PROFILE

#define PROFILE_ALLOC(...)
#define PROFILE_FREE(...)

#define PROFILE_EVENT(...)
#define PROFILE_EVENT_N(...)
#define PROFILE_EVENT_NC(...)
#define PROFILE_TAG(...)

#define PROFILE_FRAME(...)

#define PROFILE_THREAD(...)
#define PROFILE_START_THREAD(...)
#define PROFILE_STOP_THREAD(...)

#define PROFILE_SHUTDOWN(...)

#define PROFILE_GPU_INIT(...)
#define PROFILE_GPU_CONTEXT(...)
#define PROFILE_GPU_EVENT(...)
#define PROFILE_GPU_COLLECT(...)
#define PROFILE_GPU_PRESENT(...)

#define PROFILE_MESSAGE(...)
#define PROFILE_MESSAGE_DYN(...)

#define PROFILE_PLOT(...)

#define PROFILE_LOCKABLE(var, name) var name

#endif // UGINE_PROFILE

#define UGINE_GPU_EVENT(CMD, LABEL, NAME)                                                                                                                      \
    PROFILE_GPU_EVENT(CMD, NAME);                                                                                                                              \
    gfxapi::GPUDebugLabel LABEL { CMD, NAME }

#define UGINE_GPU_EVENT_C(CMD, LABEL, NAME, R, G, B)                                                                                                           \
    PROFILE_GPU_EVENT(CMD, NAME);                                                                                                                              \
    gfxapi::GPUDebugLabel LABEL { CMD, NAME, R, G, B }

namespace ugine {

class ScopeTimer {
public:
    ScopeTimer(StringView text)
        : text_{ text }
        , start_{ Clock::now() } {}

    ~ScopeTimer() { UGINE_DEBUG("{}: {}", text_, FormatTime(Clock::now() - start_)); }

private:
    using Clock = std::chrono::high_resolution_clock;

    String text_;
    Clock::time_point start_;
};

} // namespace ugine
