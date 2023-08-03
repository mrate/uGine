#pragma once

#include <gfxapi/CommandList.h>
#include <ugine/Vector.h>

#include <condition_variable>
#include <functional>
#include <mutex>

namespace ugine {

class RenderThread {
public:
    explicit RenderThread(gfxapi::Device& device);
    ~RenderThread();

    void NextFrame(bool exit = false);
    void WaitSubmit();

    template <typename T> void PushRenderWork(T&& work) { renderQueue_[SubmitQueue()].PushBack(std::move(work)); }

private:
    bool WaitFrameStart();
    void RenderThreadLoop();
    void RenderDone();
    void RenderQueue();

    using RenderWork = std::function<void(gfxapi::CommandList&)>;
    using RenderWorkQueue = Vector<RenderWork>;

    gfxapi::Device& device_;

    std::thread renderThread_;
    std::array<RenderWorkQueue, 2> renderQueue_;
    int renderingQueue_;

    int RenderingQueue() const { return renderingQueue_; }
    int SubmitQueue() const { return 1 - renderingQueue_; }

    std::condition_variable nextFrameCV_;
    std::condition_variable submitReadyCV_;

    std::mutex frameMutex_;

    bool exit_{};
    bool newFrameReady_{};
    bool submitReady_{};
};

} // namespace ugine