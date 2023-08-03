#pragma once

#include <ugine/String.h>
#include <ugine/Ugine.h>

#include <functional>

namespace ugine {

class Thread {
public:
    struct ThreadTask {
        String name;
        std::function<void()> task;
    };

    enum class Priority {
        Low,
        Normal,
        High,
    };

    static u64 AffinityForCpu(u8 cpu) { return (1ull) << cpu; }
    static u8 HardwareConcurency();

    Thread() = default;

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Thread(Thread&&);
    Thread& operator=(Thread&&);

    template <typename F>
    Thread(const char* name, u64 affinityMask, F&& func, Priority priority = Priority::Normal, IAllocator& allocator = IAllocator::Default()) {
        Create(allocator, name, priority, affinityMask, func);
    }
    template <typename F> Thread(const char* name, F&& func, Priority priority = Priority::Normal, IAllocator& allocator = IAllocator::Default()) {
        Create(allocator, name, priority, u64(-1), func);
    }
    ~Thread();

    bool Joinable() { return impl_ != nullptr; }
    void Join();
    void Detach();
    u32 Id() const { return id_; }
    u32 CpuId() const;

    void SetPriority(Priority priority);

private:
    void Create(IAllocator& allocator, const char* name, Priority priority, u64 affinityMask, std::function<void()> func);

    void* impl_{};
    UniquePtr<ThreadTask> task_;
    u32 id_{};
};

} // namespace ugine