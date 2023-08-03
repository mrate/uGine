#pragma once

#include <ugine/Locking.h>
#include <ugine/Ugine.h>

#include <array>
#include <atomic>
#include <utility>

#define UGINE_NEW(ALLOCATOR, TYPE, ...)                                                                                                                        \
    new ((ALLOCATOR).AlignedAlloc(sizeof(TYPE), alignof(TYPE))) TYPE { __VA_ARGS__ }
#define UGINE_DELETE(ALLOCATOR, PTR) (ALLOCATOR).AlignedFree(PTR)

namespace ugine {

namespace detail {
    template <class T> constexpr T& FUN(T& t) noexcept {
        return t;
    }
    template <class T> void FUN(T&&) = delete;
} // namespace detail

struct MemoryLayout {
    u32 pageSize{};
    u32 allocationGranularity{};
};

MemoryLayout GetMemoryLayout();

template <typename T> class Ref {
public:
    using Type = T;

    template <class U, class = decltype(detail::FUN<T>(std::declval<U>()), std::enable_if_t<!std::is_same_v<Ref, std::remove_cvref_t<U>>>())>
    constexpr Ref(U&& u) noexcept(noexcept(detail::FUN<T>(std::forward<U>(u))))
        : ref_(std::addressof(detail::FUN<T>(std::forward<U>(u)))) {}
    Ref(const Ref&) noexcept = default;
    Ref(Ref&& other) noexcept = default;
    Ref& operator=(const Ref&) noexcept = default;
    Ref& operator=(Ref&&) noexcept = default;
    constexpr operator T&() const noexcept { return *ref_; }
    constexpr T& Get() const noexcept { return *ref_; }
    constexpr T* operator->() const noexcept { return ref_; }

private:
    T* ref_{};
};

class IAllocator {
public:
    static IAllocator& Default() noexcept;
    static u32 NumAllocs() noexcept;
    static void ResetCounter() noexcept;

    virtual ~IAllocator() = default;

    virtual void* Alloc(size_t size) = 0;
    virtual void Free(void* memory) = 0;
    virtual void* Realloc(void* memory, size_t size) = 0;

    virtual void* AlignedAlloc(size_t size, size_t alignment) = 0;
    virtual void AlignedFree(void* memory) = 0;
    virtual void* AlignedRealloc(void* memory, size_t size, size_t alignment) = 0;

    template <typename T, typename... Args> T* New(Args&&... args) { return new (Alloc(sizeof(T))) T{ std::forward<Args>(args)... }; }
    template <typename T, typename... Args> T* AlignedNew(Args&&... args) { return new (AlignedAlloc(sizeof(T), alignof(T))) T{ std::forward<Args>(args)... }; }
};

using AllocatorRef = Ref<IAllocator>;

class CountedAllocator : public IAllocator {
public:
    explicit CountedAllocator(IAllocator& allocator)
        : allocator_{ allocator } {}
    ~CountedAllocator();

    void* Alloc(size_t size) override;
    void Free(void* memory) override;
    void* Realloc(void* memory, size_t size) override;

    void* AlignedAlloc(size_t size, size_t alignment) override;
    void AlignedFree(void* memory) override;
    void* AlignedRealloc(void* memory, size_t size, size_t alignment) override;

    u64 Count() const { return counter_; }

private:
    AllocatorRef allocator_;
    std::atomic_uint64_t counter_{};
};

class StackTraceAllocator : public IAllocator {
public:
    StackTraceAllocator(IAllocator& allocator, bool byFileName = false);
    ~StackTraceAllocator();

    void* Alloc(size_t size) override;
    void Free(void* memory) override;
    void* Realloc(void* memory, size_t size) override;

    void* AlignedAlloc(size_t size, size_t alignment) override;
    void AlignedFree(void* memory) override;
    void* AlignedRealloc(void* memory, size_t size, size_t alignment) override;

    void DumpAllocations();
    u32 ActiveAllocs();

private:
    static constexpr auto STACK_SIZE{ 256 };

    struct StackInfo {
        struct Head {
            StackInfo* next{};
            StackInfo* prev{};
        } head;

        std::array<u8, STACK_SIZE - sizeof(Head)> stack;
    };

    void AddPtr(void* ptr);
    void RemovePtr(void* ptr);

    AllocatorRef allocator_;
    AtomicSpinLock mutex_;
    StackInfo* tail_{};
    bool byFileName_{};
};

template <typename T> class UniquePtr final {
public:
    using ValueType = T;

    UniquePtr() = default;
    explicit UniquePtr(nullptr_t) noexcept {}
    ~UniquePtr() { Reset(); }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(IAllocator& allocator, T* ptr) noexcept
        : allocator_{ &allocator }
        , ptr_{ ptr } {}

    UniquePtr(UniquePtr&& other) noexcept
        : allocator_{ other.allocator_ }
        , ptr_{ std::exchange(other.ptr_, nullptr) } {}

    void operator=(UniquePtr&& other) {
        Reset();
        allocator_ = other.allocator_;
        ptr_ = std::exchange(other.ptr_, nullptr);
    }

    template <typename R> UniquePtr(UniquePtr<R>&& other) {
        allocator_ = other.GetAllocator();
        ptr_ = static_cast<T*>(std::exchange(other.ptr_, nullptr));
    }

    void operator=(nullptr_t) { Reset(); }

    template <typename R> void operator=(UniquePtr<R>&& other) {
        Reset();
        allocator_ = other.allocator_;
        ptr_ = static_cast<T*>(std::exchange(other.ptr_, nullptr));
    }

    explicit operator bool() const { return ptr_ != nullptr; }

    T* Get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }

    IAllocator* GetAllocator() const { return allocator_; }

    void Reset() {
        if (ptr_) {
            UGINE_ASSERT(allocator_);
            ptr_->~T();
            allocator_->AlignedFree(ptr_);
            ptr_ = nullptr;
        }
    }

private:
    template <class> friend class UniquePtr;

    IAllocator* allocator_{};
    T* ptr_{};
};

template <typename T, typename... Args> UniquePtr<T> MakeUnique(IAllocator& allocator, Args&&... args) {
    return UniquePtr<T>(allocator, allocator.AlignedNew<T>(std::forward<Args>(args)...));
}

class HeapAllocator final : public IAllocator {
public:
    HeapAllocator();
    ~HeapAllocator();

    void* Alloc(size_t size) override;
    void Free(void* memory) override;
    void* Realloc(void* memory, size_t size) override;

    void* AlignedAlloc(size_t size, size_t alignment) override;
    void AlignedFree(void* memory) override;
    void* AlignedRealloc(void* memory, size_t size, size_t alignment) override;
};

class LinearAllocator : public IAllocator {
public:
    class Checkpoint {
    public:
        size_t Counter() const { return counter_; }

    private:
        friend LinearAllocator;

        explicit Checkpoint(size_t cnt)
            : counter_{ cnt } {}

        size_t counter_{};
    };

    LinearAllocator() = default;
    LinearAllocator(LinearAllocator&& other) noexcept
        : allocator_{ other.allocator_ }
        , size_{ other.size_ }
        , memory_{ other.memory_ }
        , counter_{ other.counter_.load() } {
        other.memory_ = nullptr;
        other.size_ = 0;
    }

    explicit LinearAllocator(size_t size, IAllocator& allocator = IAllocator::Default());
    ~LinearAllocator();

    void Init(size_t size, IAllocator& allocator);

    void* Alloc(size_t size) override;
    void Free(void* memory) override;
    void* Realloc(void* memory, size_t size) override;

    void* AlignedAlloc(size_t size, size_t alignment) override;
    void AlignedFree(void* memory) override;
    void* AlignedRealloc(void* memory, size_t size, size_t alignment) override;

    Checkpoint GetCheckpoint() { return Checkpoint{ counter_ }; }

    void Reset();
    void Reset(Checkpoint chk);

private:
    IAllocator* allocator_{};
    size_t size_;
    void* memory_{};
    std::atomic_size_t counter_{};
};

} // namespace ugine
