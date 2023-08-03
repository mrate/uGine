#ifdef _WIN32

#include <ugine/Profile.h>
#include <ugine/String.h>
#include <ugine/Thread.h>

#include <Windows.h>

#include <thread>

namespace ugine {

constexpr auto DEFAULT_STACK_SIZE{ 0 };

DWORD WINAPI runTask(LPVOID arg) {
    auto task{ static_cast<Thread::ThreadTask*>(arg) };

    PROFILE_START_THREAD(task->name.Data());

    task->task();
    return 0;
}

u8 Thread::HardwareConcurency() {
    // TODO:
    return std::thread::hardware_concurrency();
}

Thread::Thread(Thread&& other) {
    impl_ = other.impl_;
    id_ = other.id_;
    task_ = std::move(other.task_);

    other.impl_ = nullptr;
    other.id_ = 0;
}

Thread& Thread::operator=(Thread&& other) {
    impl_ = other.impl_;
    id_ = other.id_;
    task_ = std::move(other.task_);

    other.impl_ = nullptr;
    other.id_ = 0;

    return *this;
}

void Thread::Create(IAllocator& allocator, const char* name, Priority priority, u64 affinityMask, std::function<void()> func) {
    task_ = MakeUnique<ThreadTask>(allocator, name, func);

    DWORD id{};
    impl_ = CreateThread(nullptr, DEFAULT_STACK_SIZE, runTask, task_.Get(), CREATE_SUSPENDED, &id);
    UGINE_ASSERT(impl_);
    id_ = id;

    SetThreadAffinityMask(impl_, affinityMask);

    const auto wname{ ToWString(name) };
    SetThreadDescription(impl_, wname.c_str());
    SetPriority(priority);

    ResumeThread(impl_);
}

void Thread::Join() {
    UGINE_ASSERT(impl_);

    WaitForSingleObject(impl_, INFINITE);
    CloseHandle(impl_);
    impl_ = nullptr;
    task_ = nullptr;
}

Thread::~Thread() {
    UGINE_ASSERT(!impl_);

    if (Joinable()) {
        Join();
    }
}

void Thread::Detach() {
    UGINE_ASSERT(impl_);
    CloseHandle(impl_);
    impl_ = nullptr;
}

u32 Thread::CpuId() const {
    return GetCurrentProcessorNumber();
}

void Thread::SetPriority(Priority priority) {
    UGINE_ASSERT(impl_);

    switch (priority) {
    case Priority::Low: SetThreadPriority(impl_, THREAD_PRIORITY_BELOW_NORMAL); break;
    case Priority::Normal: SetThreadPriority(impl_, THREAD_PRIORITY_NORMAL); break;
    case Priority::High: SetThreadPriority(impl_, THREAD_PRIORITY_ABOVE_NORMAL); break;
    }
}

} // namespace ugine

#endif // _WIN32