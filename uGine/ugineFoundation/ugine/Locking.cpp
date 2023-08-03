#ifdef _WIN32
#include "Locking.h"
#include "Assert.h"

#include <Windows.h>

#include <synchapi.h>

namespace ugine {

Mutex::Mutex() {
    static_assert(sizeof(impl_) == sizeof(SRWLOCK));
    static_assert(alignof(Mutex) == alignof(SRWLOCK));

    InitializeSRWLock(reinterpret_cast<PSRWLOCK>(impl_));
}

Mutex::~Mutex() {
}

void Mutex::Lock() {
    AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(impl_));
}

void Mutex::Unlock() {
    ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(impl_));
}

Semaphore::Semaphore(u32 initCnt, u32 maxCnt) {
    UGINE_ASSERT(initCnt <= maxCnt);

    impl_ = CreateSemaphore(nullptr, initCnt, maxCnt, nullptr);

    UGINE_ASSERT(impl_);
}

Semaphore::~Semaphore() {
    CloseHandle(static_cast<HANDLE>(impl_));
}

void Semaphore::Wait() {
    // Decrement.
    WaitForSingleObject(static_cast<HANDLE>(impl_), INFINITE);
}

void Semaphore::Signal() {
    // Increment.
    const auto res{ ReleaseSemaphore(static_cast<HANDLE>(impl_), 1, nullptr) };

    UGINE_ASSERT(res);
}

CondVar::CondVar() {
    static_assert(alignof(CondVar) == alignof(CONDITION_VARIABLE));
    static_assert(sizeof(impl_) >= sizeof(CONDITION_VARIABLE));

    InitializeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(&impl_));
}

CondVar::~CondVar() {
}

void CondVar::Wait(Lock<Mutex>& lock) {
    SleepConditionVariableSRW(reinterpret_cast<PCONDITION_VARIABLE>(impl_), reinterpret_cast<PSRWLOCK>(lock.Mutex().Impl()), INFINITE, 0);
}

void CondVar::Notify() {
    WakeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(impl_));
}

} // namespace ugine

#endif // _WIN32