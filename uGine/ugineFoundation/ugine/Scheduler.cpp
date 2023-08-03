#include "Scheduler.h"

#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/Vector.h>

#include <algorithm>
#include <format>
#include <vector>

namespace ugine {

thread_local u32 ThreadNum{ u32(-1) };

void* EnkiAlloc(size_t align, size_t size, void* userData, const char* file, int line) {
    auto allocator{ static_cast<IAllocator*>(userData) };
    return allocator->AlignedAlloc(size, align);
};

void EnkiFree(void* ptr, size_t size, void* userData, const char* file, int line) {
    auto allocator{ static_cast<IAllocator*>(userData) };
    allocator->AlignedFree(ptr);
};

class ThreadInitializer final : public enki::IPinnedTask {
public:
    ThreadInitializer(u32 threadNum, const String& name)
        : enki::IPinnedTask{ threadNum }
        , name_{ name } {}

    void Execute() override {
        PROFILE_START_THREAD(name_.Data());
        ThreadNum = threadNum;
    }

private:
    String name_;
};

class ThreadDeinitializer final : public enki::IPinnedTask {
public:
    ThreadDeinitializer(u32 threadNum)
        : enki::IPinnedTask{ threadNum } {}

    void Execute() override { PROFILE_STOP_THREAD(); }
};

Scheduler::Scheduler(u32 tasks, Span<const String> taskNames, IAllocator& allocator)
    : allocator_{ allocator } {
    enki::TaskSchedulerConfig config{};
    config.customAllocator.alloc = EnkiAlloc;
    config.customAllocator.free = EnkiFree;
    config.customAllocator.userData = &allocator_.Get();
    config.numTaskThreadsToCreate = tasks;

    scheduler_.Initialize(config);

    InitThreads(taskNames);
}

Scheduler::~Scheduler() {
    scheduler_.WaitforAllAndShutdown();
    FinishThreads();
}

void Scheduler::InitThreads(Span<const String> names) {
    Vector<UniquePtr<ThreadInitializer>> tasks{ scheduler_.GetNumTaskThreads(), allocator_ };
    for (u32 i{}; i < scheduler_.GetNumTaskThreads(); ++i) {
        tasks[i] = MakeUnique<ThreadInitializer>(allocator_, i, i < names.Size() ? names[i] : "Thread");
        scheduler_.AddPinnedTask(tasks[i].Get());
    }

    std::for_each(tasks.Begin(), tasks.End(), [&](auto& t) { scheduler_.WaitforTask(t.Get()); });
}

void Scheduler::FinishThreads() {
    Vector<UniquePtr<ThreadDeinitializer>> tasks{ scheduler_.GetNumTaskThreads(), allocator_ };
    for (u32 i{}; i < scheduler_.GetNumTaskThreads(); ++i) {
        tasks[i] = MakeUnique<ThreadDeinitializer>(allocator_, i);
        scheduler_.AddPinnedTask(tasks[i].Get());
    }

    std::for_each(tasks.Begin(), tasks.End(), [&](auto& t) { scheduler_.WaitforTask(t.Get()); });
}

void Scheduler::ScheduleStatic(Group& grp, std::function<void()> func) {
    const auto index{ grp.count.fetch_add(1) };
    // TODO: Create custom inherited ITaskSet without std::function.
    auto task = new (reinterpret_cast<void*>(&grp.staticTasks[index])) enki::TaskSet{ [func](enki::TaskSetPartition t, u32 num) { func(); } };
    scheduler_.AddTaskSetToPipe(task);
}

void Scheduler::ScheduleStatic(Group& grp, u32 num, std::function<void(u32, u32, u32)> func) {
    UGINE_ASSERT(num > 0);

    const auto index{ grp.count.fetch_add(1) };
    // TODO: Create custom inherited ITaskSet without std::function.
    auto task = new (reinterpret_cast<void*>(&grp.staticTasks[index])) enki::TaskSet{ num, [func](enki::TaskSetPartition t, u32 num) { func(t.start, t.end, num); } };
    scheduler_.AddTaskSetToPipe(task);
}

void Scheduler::Schedule(Group& grp, u32 num, Task* task) {
    UGINE_ASSERT(num > 0);

    UGINE_ASSERT(task);
    const auto index{ grp.count.fetch_add(1) };
    grp.tasks[index] = task;

    scheduler_.AddTaskSetToPipe(task);
}

u32 Scheduler::Wait(Group& grp) {
    // This should wait for all tasks even if they are continously added to group.
    u32 cnt{};
    for (; cnt < grp.count.load(); ++cnt) {
        if (grp.tasks[cnt]) {
            // External tasks.
            scheduler_.WaitforTask(grp.tasks[cnt]);
        } else {
            // Statically allocated tasks.
            scheduler_.WaitforTask(&grp.staticTasks[cnt]);
            grp.staticTasks[cnt].~TaskSet();
        }
    }
    return cnt;
}

void Scheduler::Schedule(TaskSet* task) {
    scheduler_.AddTaskSetToPipe(task);
}

void Scheduler::SchedulePinned(PinnedTask* task) {
    scheduler_.AddPinnedTask(task);
}

void Scheduler::WaitFor(Completable* task) {
    scheduler_.WaitforTask(task);
}

void Scheduler::WaitForAll() {
    scheduler_.WaitforAll();
}

u32 Scheduler::NumThreads() const {
    return scheduler_.GetNumTaskThreads();
}

void Scheduler::ShutDown() {
    scheduler_.WaitforAllAndShutdown();
}

u32 Scheduler::ThreadNum() {
    return ugine::ThreadNum;
}

} // namespace ugine
