#include "Jobs.h"

#include "Log.h"

#include <format>

namespace ugine {

namespace detail {

    // https://github.com/krzysztofmarecki/JobSystem/blob/master/src/JobSystem.cpp

    void __stdcall WorkerEntryPoint(void* arg);

    // TLS:
    thread_local u8 tls_WorkerId{ 0xff };
    thread_local Fiber::Native tls_Fiber{};
    thread_local Fiber::Native tls_FinishedFiber{};

    struct WaitEntry {
        Fiber::Native fiber{};
        JobCounter* counter{};
    };

    struct Worker {
        Thread thread;
        Fiber fiber;
        u32 index{};
        JobSchedulerImpl* scheduler{};

        Worker() = default;
        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;
        Worker(Worker&& other) {
            std::swap(thread, other.thread);
            std::swap(fiber, other.fiber);
            std::swap(index, other.index);
            std::swap(scheduler, other.scheduler);
        }
        Worker& operator=(Worker&& other) {
            std::swap(thread, other.thread);
            std::swap(fiber, other.fiber);
            std::swap(index, other.index);
            std::swap(scheduler, other.scheduler);
            return *this;
        }

        ~Worker() { thread.Join(); }
    };

    class JobSchedulerImpl {
    public:
        static constexpr u32 FIBER_POOL_SIZE{ 128 };
        static constexpr u32 FIBER_STACK_SIZE{ 64 * 1024 };

        using JobQueue = ConcurentRingbuffer<Job, JobScheduler::MAX_PENDING_JOBS, AtomicSpinLock>;

        JobSchedulerImpl(IAllocator& allocator, u8 workers)
            : workers_{ allocator }
            , waitList_{ allocator } {

            mainFiber_ = Fiber::ThreadToFiber(this);

            running_ = true;

            for (int i{}; i < FIBER_POOL_SIZE; ++i) {
                fiberPool_.PushBack(Fiber{ FIBER_STACK_SIZE, WorkerEntryPoint, this }.DetachNative());
            }

            workers_.Resize(workers);
            for (u8 i{ 1 }; i < workers - 1; ++i) {
                workers_[i].index = i;
                workers_[i].scheduler = this;
                workers_[i].thread = Thread{
                    std::format("JobScheduler#{}", i).c_str(),
                    Thread::AffinityForCpu(i),
                    [this, i] { WorkerThread(i); },
                    Thread::Priority::Normal,
                    allocator,
                };
            };
        }

        ~JobSchedulerImpl() {
            running_ = false;

            for (int i{}; i < workers_.Size(); ++i) {
                workerSemaphore_.Signal();
            }

            workers_.Clear();
        }

        void AddJob(const Job& job) {
            jobQueue_[int(job.priority)].PushBack(job);
            workerSemaphore_.Signal();
        }

        void Wait(JobCounter* counter) {
            WaitEntry wait{
                .fiber = tls_Fiber,
                .counter = counter,
            };

            {
                Lock lock{ waitListLock_ };
                waitList_.PushBack(wait);
            }

            Fiber::Native newFiber{};
            if (!fiberPool_.PopFront(newFiber)) {
                UGINE_ASSERT(false);
            }

            tls_Fiber = newFiber;
            Fiber::SwitchToNativeFiber(tls_Fiber);

            UGINE_ASSERT(tls_Fiber != nullptr);
            UGINE_ASSERT(tls_FinishedFiber != nullptr);

            fiberPool_.PushBack(tls_FinishedFiber);
            tls_FinishedFiber = nullptr;
        }

        void WorkerThread(u8 id) {
            UGINE_DEBUG("STARTING THREAD {}", id);

            auto& worker{ workers_[id] };
            worker.fiber = Fiber::ThreadToFiber(nullptr);

            tls_WorkerId = id;
            tls_Fiber = worker.fiber.GetNative();

            WorkerEntryPoint(this);
        }

        bool PopJob(Job& job) {
            for (auto priority : { JobPriority::High, JobPriority::Normal, JobPriority::Low }) {
                if (jobQueue_[int(priority)].PopFront(job)) {
                    return true;
                }
            }

            return false;
        }

        void HandleJob(Job& job) {
            job.func(job.arg);

            if (job.counter == nullptr) {
                return;
            }

            if (--job.counter->counter > 0) {
                return;
            }

            WaitEntry wait{};
            {
                Lock lock{ waitListLock_ };
                const auto index{ waitList_.FindIf([&](const auto& entry) { return entry.counter == job.counter; }) };
                if (index >= 0) {
                    wait = waitList_[index];
                    waitList_.EraseAt(index);
                }
            }

            tls_FinishedFiber = tls_Fiber;
            tls_Fiber = wait.fiber;
            Fiber::SwitchToNativeFiber(tls_Fiber);
        }

        Fiber mainFiber_;
        std::atomic_bool running_{};

        // Workers.
        Vector<Worker> workers_;
        Semaphore workerSemaphore_{ 0, JobScheduler::MAX_PENDING_JOBS };

        // Fiber pool.
        ConcurentRingbuffer<Fiber::Native, FIBER_POOL_SIZE> fiberPool_;

        // Work.
        std::array<JobQueue, int(JobPriority::COUNT)> jobQueue_;

        Vector<WaitEntry> waitList_;
        AtomicSpinLock waitListLock_;
    };

    void __stdcall WorkerEntryPoint(void* arg) {
        auto scheduler{ static_cast<JobSchedulerImpl*>(arg) };

        while (scheduler->running_) {
            scheduler->workerSemaphore_.Wait();

            if (!scheduler->running_) {
                break;
            }

            Job job{};
            if (!scheduler->PopJob(job)) {
                continue;
            }

            scheduler->HandleJob(job);
        }

        Fiber::FiberToThread();
    }

} // namespace detail

JobScheduler::JobScheduler() {
}

JobScheduler::~JobScheduler() {
}

void JobScheduler::Init(IAllocator& allocator, u8 workers) {
    UGINE_ASSERT(!impl_);

    impl_ = MakeUnique<detail::JobSchedulerImpl>(allocator, allocator, workers);
}

void JobScheduler::Shutdown() {
    UGINE_ASSERT(impl_);

    impl_ = nullptr;
}

void JobScheduler::AddJob(const Job& job) {
    impl_->AddJob(job);
}

void JobScheduler::Wait(JobCounter* counter) {
    impl_->Wait(counter);
}

} // namespace ugine