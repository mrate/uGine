#pragma once

#include <ugine/Locking.h>

#include <entt/entt.hpp>

namespace ugine {

template <typename _LockingPolicy> class BaseEventEmittor {
public:
    using Locking = _LockingPolicy;

    template <typename Event, auto Handler, typename... Args> void Connect(Args&&... args) {
        typename Locking::WriteScopeLock lock{ locking_.Mutex() };
        disp_.sink<Event>().connect<Handler>(std::forward<Args>(args)...);
    }

    template <typename Event, typename... Args> void Disconnect(Args&&... args) {
        typename Locking::WriteScopeLock lock{ locking_.Mutex() };
        disp_.sink<Event>().disconnect(std::forward<Args>(args)...);
    }

protected:
    template <typename... Args> void Emit(Args&&... args) {
        typename Locking::WriteScopeLock lock{ locking_.Mutex() };
        disp_.trigger(std::forward<Args>(args)...);
    }
    template <typename... Args> void Enqueue(Args&&... args) {
        typename Locking::WriteScopeLock lock{ locking_.Mutex() };
        disp_.enqueue(std::forward<Args>(args)...);
    }

    template <typename Event> void Update() {
        typename Locking::WriteScopeLock lock{ locking_.Mutex() };
        disp_.update<Event>();
    }

    bool Update() {
        typename Locking::WriteScopeLock lock{ locking_.Mutex() };
        disp_.update();
        return disp_.size() > 0;
    }

private:
    Locking locking_;
    entt::dispatcher disp_;
};

using EventEmittor = BaseEventEmittor<NoLockingPolicy>;
using EventEmittorMT = BaseEventEmittor<LockingPolicy<AtomicSpinLock>>;

} // namespace ugine