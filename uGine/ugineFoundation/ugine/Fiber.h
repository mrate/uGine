#pragma once

#include <ugine/Ugine.h>

#include <atomic>

namespace ugine {

using FiberFunc = void(__stdcall*)(void*);

class Fiber {
public:
    using Native = void*;

    static Fiber ThreadToFiber(void* arg);
    static void FiberToThread();
    static Fiber FromNative(Native ptr) { return Fiber{ ptr }; }

    static void SwitchToNativeFiber(Native fiber);

    Fiber() = default;
    Fiber(u32 stackSize, FiberFunc func, void* arg);
    ~Fiber();

    Fiber(const Fiber&) = delete;
    Fiber& operator=(const Fiber&) = delete;

    Fiber(Fiber&& other);
    Fiber& operator=(Fiber&& other);

    Fiber::Native GetNative() const { return impl_; }
    Fiber::Native DetachNative() {
        auto native{ impl_ };
        impl_ = nullptr;
        return impl_;
    }

    void SwitchTo();

    operator bool() const { return impl_ != nullptr; }

private:
    Fiber(Native impl)
        : impl_{ impl } {}

    Native impl_{};
};

} // namespace ugine