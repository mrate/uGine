#ifdef _WIN32

#include "Fiber.h"
#include "Assert.h"

#include <Windows.h>

namespace ugine {

Fiber Fiber::ThreadToFiber(void* arg) {
    return Fiber{ ::ConvertThreadToFiber(arg) };
}

void Fiber::FiberToThread() {
    const auto res{ ::ConvertFiberToThread() };
    UGINE_ASSERT(res);
}

void Fiber::SwitchToNativeFiber(Native fiber) {
    ::SwitchToFiber(fiber);
}

Fiber::Fiber(u32 stackSize, FiberFunc func, void* arg) {
    impl_ = ::CreateFiber(stackSize, func, arg);

    UGINE_ASSERT(impl_);
}

Fiber::Fiber(Fiber&& other)
    : impl_{ other.impl_ } {
    other.impl_ = nullptr;
}

Fiber& Fiber::operator=(Fiber&& other) {
    impl_ = other.impl_;
    other.impl_ = nullptr;

    return *this;
}

Fiber::~Fiber() {
    if (impl_) {
        ::DeleteFiber(impl_);
        impl_ = nullptr;
    }
}

void Fiber::SwitchTo() {
    ::SwitchToFiber(impl_);
}

} // namespace ugine

#endif // _WIN32