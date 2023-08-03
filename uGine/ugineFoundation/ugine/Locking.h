#pragma once

#include <ugine/Ugine.h>

#include <atomic>
#include <thread>

namespace ugine {

class AtomicSpinLock {
public:
    AtomicSpinLock(const AtomicSpinLock&) = delete;
    AtomicSpinLock& operator=(const AtomicSpinLock&) = delete;

    AtomicSpinLock() {}

    void Lock() {
        while (lock_.test_and_set(std::memory_order_acquire))
            ;

#ifdef _DEBUG
        threadId_ = std::this_thread::get_id();
#endif
    }

    void Unlock() {
#ifdef _DEBUG
        threadId_ = std::thread::id{};
#endif

        lock_.clear(std::memory_order_release);
    }

private:
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

#ifdef _DEBUG
    std::thread::id threadId_{};
#endif
};

class alignas(8) Mutex {
public:
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    Mutex();
    ~Mutex();

    void Lock();
    void Unlock();

    void lock() { Lock(); }
    void unlock() { Unlock(); }

private:
    friend class CondVar;

    void* Impl() { return impl_; }

    u8 impl_[8];
};

class Semaphore {
public:
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&&) = delete;
    Semaphore& operator=(Semaphore&&) = delete;

    Semaphore(u32 initCnt, u32 maxCnt);
    ~Semaphore();

    void Wait();
    void Signal();

private:
    void* impl_{};
};

template <typename T> class Lock {
public:
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;

    Lock(T& t)
        : lockable_{ t } {
        lockable_.Lock();
    }

    ~Lock() { lockable_.Unlock(); }

    T& Mutex() { return lockable_; }

private:
    T& lockable_;
};

class alignas(8) CondVar {
public:
    CondVar(const CondVar&) = delete;
    CondVar& operator=(const CondVar&) = delete;
    CondVar(CondVar&&) = delete;
    CondVar& operator=(CondVar&&) = delete;

    CondVar();
    ~CondVar();

    void Wait(Lock<Mutex>& lock);

    template <typename Pred> void Wait(Lock<Mutex>& lock, Pred stopWait) {
        for (;;) {
            Wait(lock);
            if (stopWait()) {
                break;
            }
        }
    }

    void Notify();

private:
    u8 impl_[8];
};

struct NoLockingPolicy {
    struct Empty {};

    struct ReadScopeLock {
        ReadScopeLock(Empty) {}
    };
    struct WriteScopeLock {
        WriteScopeLock(Empty) {}
    };

    Empty Mutex() { return {}; }
};

template <typename _MutexType> struct LockingPolicy {
    using MutexType = _MutexType;
    using ReadScopeLock = Lock<MutexType>;
    using WriteScopeLock = Lock<MutexType>;

    MutexType& Mutex() { return mutex_; }
    MutexType mutex_;
};

} // namespace ugine
