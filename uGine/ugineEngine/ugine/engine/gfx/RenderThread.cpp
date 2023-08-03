#include "RenderThread.h"

#include <ugine/Profile.h>

#include <gfxapi/Device.h>

namespace ugine {

RenderThread::RenderThread(gfxapi::Device& device)
    : device_{ device } {
    renderThread_ = std::thread([this] { RenderThreadLoop(); });
}

RenderThread::~RenderThread() {
    NextFrame(true);

    if (renderThread_.joinable()) {
        renderThread_.join();
    }
}

bool RenderThread::WaitFrameStart() {
    PROFILE_EVENT_N("WaitFrameStart");

    std::unique_lock lock{ frameMutex_ };

    nextFrameCV_.wait(lock, [this] { return exit_ || newFrameReady_; });
    newFrameReady_ = false;
    renderingQueue_ = 1 - renderingQueue_;

    return !exit_;
}

void RenderThread::RenderThreadLoop() {
    PROFILE_THREAD("RenderThread");

    while (!exit_) {
        if (!WaitFrameStart()) {
            break;
        }

        RenderQueue();

        RenderDone();
    }
}

void RenderThread::RenderDone() {
    PROFILE_EVENT_N("RenderDone");

    {
        std::scoped_lock lock{ frameMutex_ };
        submitReady_ = true;
    }

    submitReadyCV_.notify_all();
}

void RenderThread::RenderQueue() {
    PROFILE_EVENT_N("RenderQueue");

    auto cmd{ device_.BeginCommandList() };

    auto& queue{ renderQueue_[RenderingQueue()] };

    for (auto& command : queue) {
        command(*cmd);
    }

    device_.SubmitCommandLists();

    queue.Clear();
}

void RenderThread::NextFrame(bool exit) {
    {
        std::unique_lock lock{ frameMutex_ };
        exit_ = exit;
        newFrameReady_ = true;
    }

    nextFrameCV_.notify_all();
}

void RenderThread::WaitSubmit() {
    std::unique_lock lock{ frameMutex_ };

    submitReadyCV_.wait(lock, [this] { return submitReady_; });
    submitReady_ = false;
}

} // namespace ugine