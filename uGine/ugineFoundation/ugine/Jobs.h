#pragma once

#include "Concurrent.h"
#include "Fiber.h"
#include "Locking.h"
#include "Memory.h"
#include "Thread.h"
#include "Ugine.h"
#include "Vector.h"

#include <atomic>

namespace ugine {

enum class JobPriority {
    Low = 0,
    Normal,
    High,
    COUNT,
};

struct JobCounter {
    std::atomic_uint32_t counter{};
};

struct Job {
    using Func = void (*)(void*);

    Func func{};
    void* arg{};
    JobPriority priority{ JobPriority::Normal };
    JobCounter* counter{};
};

namespace detail {
    class JobSchedulerImpl;
}

class JobScheduler {
public:
    static constexpr u32 MAX_PENDING_JOBS{ 4096 };

    JobScheduler();
    ~JobScheduler();

    void Init(IAllocator& allocator, u8 workers);
    void Shutdown();

    void AddJob(const Job& job);
    void Wait(JobCounter* counter);

private:
    UniquePtr<detail::JobSchedulerImpl> impl_;
};

} // namespace ugine