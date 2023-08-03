#pragma once

#include "Assert.h"
#include "Locking.h"
#include "Memory.h"
#include "Vector.h"

#include <array>
#include <atomic>

namespace ugine {

// Based on lockfree stack from Intrinsic engine: https://github.com/begla/Intrinsic
template <class T> class LockFreeStack {
public:
    LockFreeStack(IAllocator& allocator, u32 capacity)
        : allocator_{ allocator } {
        data_ = allocator.AlignedAlloc(capacity * sizeof(T), alignof(T));
        capacity_ = capacity;
        size_ = 0u;
    }

    ~LockFreeStack() { allocator_.Free(data_); }

    void push_back(const T& element) {
        auto oldSize{ size_.fetch_add(1) };
        UGINE_ASSERT(oldSize + 1u <= capacity_ && "Stack overflow");
        data_[oldSize] = element;
    }

    void pop_back() {
        const auto oldSize{ size_.fetch_sub(1) };
        UGINE_ASSERT(oldSize - 1u <= capacity_ && "Stack underflow");
    }

    void resize(u64 size) { size_ = size; }
    void clear() { resize(0u); }
    bool empty() const { return size_ == 0u; }

    T& back() {
        UGINE_ASSERT(!empty());
        return data_[size_ - 1u];
    }

    const T& back() const {
        UGINE_ASSERT(!empty());
        return data_[size_ - 1u];
    }

    u64 capacity() const { return capacity_; }
    u64 size() const { return size_; }

    T& operator[](u64 idx) { return data_[idx]; }
    T& operator[](u64 idx) const { return data_[idx]; }

    void insert(const Vector<T>& vals) {
        const std::atomic_uint64_t oldSize = size_.fetch_add(vals.size());
        UGINE_ASSERT(oldSize + vals.size() <= capacity_);
        memcpy(&data_[oldSize], vals.data(), vals.size() * sizeof(T));
    }

    template <typename Container> void copy(Container& vals) const {
        const u32 startIdx = static_cast<u32>(vals.size());
        vals.resize(vals.size() + size_);
        memcpy(&vals.data()[startIdx], data_, size_ * sizeof(T));
    }

private:
    AllocatorRef allocator_;
    T* data_{};
    u64 capacity_;
    std::atomic_uint64_t size_;
};

template <typename _T, u32 _Capacity, typename _MutexType = AtomicSpinLock> class ConcurentRingbuffer {
public:
    static const u32 Capacity{ _Capacity };
    using MutexType = _MutexType;
    using ValueType = _T;

    inline bool PushBack(const ValueType& item) {
        bool result{};

        Lock lock{ mutex_ };
        u32 next = (head_ + 1) % Capacity;
        if (next != tail_) {
            data_[head_] = item;
            head_ = next;
            result = true;
        }

        return result;
    }

    inline bool PopFront(ValueType& item) {
        bool result{};

        Lock lock{ mutex_ };
        if (tail_ != head_) {
            item = data_[tail_];
            tail_ = (tail_ + 1) % Capacity;
            result = true;
        }

        return result;
    }

private:
    std::array<ValueType, Capacity> data_;
    u32 head_{};
    u32 tail_{};
    MutexType mutex_{};
};

} // namespace ugine