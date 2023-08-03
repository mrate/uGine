#pragma once

#include <ugine/Delegate.h>
#include <ugine/Span.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>

#include <TaskScheduler.h>

#include <atomic>

namespace ugine {

using PinnedTask = enki::IPinnedTask;
using TaskSet = enki::ITaskSet;
using Completable = enki::ICompletable;
using TaskSetPartition = enki::TaskSetPartition;

class Task : public enki::ITaskSet {
public:
    Task(u32 size)
        : enki::ITaskSet{ size } {}
    Task(u32 size, u32 minRange)
        : enki::ITaskSet{ size, minRange } {}

    void ExecuteRange(TaskSetPartition range, u32 threadnum) override { Run(range.start, range.end, threadnum); }

    virtual void Run(u32 start, u32 end, u32 threadNum) = 0;
};

class Scheduler {
public:
    struct Group {
        std::atomic_uint32_t count;
        std::array<enki::ITaskSet*, 2048> tasks;
        std::array<enki::TaskSet, 2048> staticTasks;

        void Reset() { count = 0; }
    };

    static u32 ThreadNum();

    Scheduler(u32 tasks, Span<const String> taskNames = {}, IAllocator& allocator = IAllocator::Default());
    ~Scheduler();

    void ScheduleStatic(Group& grp, std::function<void()> func);
    void ScheduleStatic(Group& grp, u32 num, std::function<void(u32 /*start*/, u32 /*end*/, u32 /*threadNum*/)> func);
    void Schedule(Group& grp, u32 num, Task* task);
    
    u32 Wait(Group& grp);

    void Schedule(TaskSet* task);
    void SchedulePinned(PinnedTask* task);
    void WaitFor(Completable* task);
    void WaitForAll();
    u32 NumThreads() const;

    void ShutDown();

    enki::TaskScheduler& GetScheduler() { return scheduler_; }

private:
    void InitThreads(Span<const String> taskNames);
    void FinishThreads();

    AllocatorRef allocator_;
    enki::TaskScheduler scheduler_;
};

} // namespace ugine
