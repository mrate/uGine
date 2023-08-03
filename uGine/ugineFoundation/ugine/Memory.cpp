#include "Memory.h"

#include <ugine/Align.h>
#include <ugine/Locking.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/StackTrace.h>
#include <ugine/Ugine.h>

#include <mimalloc.h>

#ifdef WIN32
#include <Windows.h>
#endif

#include <iostream>

void* operator new(std::size_t count) {
    return ugine::IAllocator::Default().Alloc(count);
}
void operator delete(void* ptr) noexcept {
    ugine::IAllocator::Default().Free(ptr);
}

namespace ugine {

std::atomic_uint32_t numAllocs{};

//
// mimalloc allocator.
////////////////////////////////////////////////

class MimallocAllocator final : public IAllocator {
public:
    MimallocAllocator() = default;
    ~MimallocAllocator() = default;

    void* Alloc(size_t size) override {
        ++numAllocs;

        auto ptr{ size <= MI_SMALL_SIZE_MAX ? mi_malloc_small(size) : mi_malloc(size) };
        PROFILE_ALLOC(ptr, size);

        return ptr;
    }

    void* Realloc(void* memory, size_t size) override {
        ++numAllocs;

        UGINE_ASSERT(size > 0);
        PROFILE_FREE(memory);
        auto ptr{ mi_realloc(memory, size) };
        PROFILE_ALLOC(ptr, size);

        return ptr;
    }

    void Free(void* memory) override {
        PROFILE_FREE(memory);
        mi_free(memory);
    }

    void* AlignedAlloc(size_t size, size_t alignment) override {
        ++numAllocs;

        auto ptr{ mi_malloc_aligned(size, alignment) };
        PROFILE_ALLOC(ptr, size);

        return ptr;
    }

    void* AlignedRealloc(void* memory, size_t size, size_t alignment) override {
        ++numAllocs;

        UGINE_ASSERT(size > 0);
        PROFILE_FREE(memory);
        auto ptr{ mi_realloc_aligned(memory, size, alignment) };
        PROFILE_ALLOC(ptr, size);

        return ptr;
    }

    void AlignedFree(void* memory) override {
        PROFILE_FREE(memory);
        //mi_free_aligned(memory);
        mi_free(memory);
    }
};

//
// Debug allocators.
////////////////////////////////////////////////

CountedAllocator::~CountedAllocator() {
    UGINE_ASSERT(counter_ == 0 && ":-(");
}

void* CountedAllocator::Alloc(size_t size) {
    ++counter_;
    return allocator_->Alloc(size);
}

void CountedAllocator::Free(void* memory) {
    if (memory) {
        --counter_;
        allocator_->Free(memory);
    }
}

void* CountedAllocator::Realloc(void* memory, size_t size) {
    if (memory && size == 0) {
        --counter_;
    } else if (memory == nullptr && size > 0) {
        ++counter_;
    }
    return allocator_->Realloc(memory, size);
}

void* CountedAllocator::AlignedAlloc(size_t size, size_t alignment) {
    ++counter_;
    return allocator_->AlignedAlloc(size, alignment);
}

void CountedAllocator::AlignedFree(void* memory) {
    if (memory) {
        --counter_;
        allocator_->AlignedFree(memory);
    }
}

void* CountedAllocator::AlignedRealloc(void* memory, size_t size, size_t alignment) {
    if (memory && size == 0) {
        --counter_;
    } else if (memory == nullptr && size > 0) {
        ++counter_;
    }
    return allocator_->AlignedRealloc(memory, size, alignment);
}

//
// StackTrace allocator.
////////////////////////////////////////////////
StackTraceAllocator::StackTraceAllocator(IAllocator& allocator, bool byFileName)
    : allocator_{ allocator }
    , byFileName_{ byFileName } {
}

StackTraceAllocator::~StackTraceAllocator() {
    DumpAllocations();

    UGINE_ASSERT(tail_ == nullptr);
}

void StackTraceAllocator::DumpAllocations() {
    auto ptr{ tail_ };
    while (ptr) {
        std::cerr << "------- ACTIVE ALLOC: ------\n" << reinterpret_cast<const char*>(ptr->stack.data()) << std::endl;
        ptr = ptr->head.prev;
    }
}

void* StackTraceAllocator::Alloc(size_t size) {
    auto ptr{ allocator_->Alloc(size + sizeof(StackInfo)) };
    auto stack{ reinterpret_cast<StackInfo*>(ptr) };
    CaptureStackTraceCompressed(Span{ stack->stack.data(), stack->stack.size() }, 2, byFileName_);
    AddPtr(ptr);
    return reinterpret_cast<u8*>(ptr) + sizeof(StackInfo);
}

void StackTraceAllocator::Free(void* memory) {
    if (memory) {
        auto ptr{ reinterpret_cast<u8*>(memory) - sizeof(StackInfo) };
        RemovePtr(ptr);
        allocator_->Free(ptr);
    }
}

void* StackTraceAllocator::Realloc(void* memory, size_t size) {
    if (memory) {
        auto ptr{ reinterpret_cast<u8*>(memory) - sizeof(StackInfo) };
        RemovePtr(ptr);
    }

    auto ptr{ allocator_->Realloc(memory ? reinterpret_cast<u8*>(memory) - sizeof(StackInfo) : nullptr, size + sizeof(StackInfo)) };
    auto stack{ reinterpret_cast<StackInfo*>(ptr) };
    CaptureStackTraceCompressed(Span{ stack->stack.data(), stack->stack.size() }, 2, byFileName_);
    AddPtr(ptr);
    return reinterpret_cast<u8*>(ptr) + sizeof(StackInfo);
}

void* StackTraceAllocator::AlignedAlloc(size_t size, size_t alignment) {
    const auto alignedStackSize{ AlignTo(sizeof(StackInfo) + 8, alignment) };
    auto ptr{ allocator_->AlignedAlloc(size + alignedStackSize, alignment) };
    auto stack{ reinterpret_cast<StackInfo*>(ptr) };
    CaptureStackTraceCompressed(Span{ stack->stack.data(), stack->stack.size() }, 2, byFileName_);
    AddPtr(ptr);
    ptr = reinterpret_cast<u8*>(ptr) + alignedStackSize;
    *(reinterpret_cast<u64*>(ptr) - 1) = u64(alignedStackSize);
    return ptr;
}

void StackTraceAllocator::AlignedFree(void* memory) {
    if (memory) {
        const auto alignedStackSize{ *(reinterpret_cast<u64*>(memory) - 1) };
        auto ptr{ reinterpret_cast<u8*>(memory) - alignedStackSize };
        RemovePtr(ptr);
        allocator_->AlignedFree(ptr);
    }
}

void* StackTraceAllocator::AlignedRealloc(void* memory, size_t size, size_t alignment) {
    if (memory) {
        auto ptr{ reinterpret_cast<u8*>(memory) - 8 };
        const auto alignedStackSize{ *reinterpret_cast<u64*>(ptr) };
        memory = reinterpret_cast<u8*>(memory) - alignedStackSize;
        RemovePtr(memory);
    }

    const auto alignedStackSize{ AlignTo(sizeof(StackInfo) + 8, alignment) };
    auto ptr{ allocator_->AlignedRealloc(memory, size + alignedStackSize, alignment) };
    auto stack{ reinterpret_cast<StackInfo*>(ptr) };
    CaptureStackTraceCompressed(Span{ stack->stack.data(), stack->stack.size() }, 2, byFileName_);
    AddPtr(ptr);
    ptr = reinterpret_cast<u8*>(ptr) + alignedStackSize;
    *(reinterpret_cast<u64*>(ptr) - 1) = u64(alignedStackSize);
    return ptr;
}

void StackTraceAllocator::AddPtr(void* ptr) {
    auto stackInfo{ reinterpret_cast<StackInfo*>(ptr) };

    Lock lock{ mutex_ };
    stackInfo->head.next = nullptr;
    stackInfo->head.prev = tail_;
    if (tail_) {
        tail_->head.next = stackInfo;
    }
    tail_ = stackInfo;
}

void StackTraceAllocator::RemovePtr(void* ptr) {
    auto stackInfo{ reinterpret_cast<StackInfo*>(ptr) };

    Lock lock{ mutex_ };
    if (stackInfo->head.prev) {
        stackInfo->head.prev->head.next = stackInfo->head.next;
    }
    if (stackInfo->head.next) {
        stackInfo->head.next->head.prev = stackInfo->head.prev;
    }
    if (tail_ == stackInfo) {
        tail_ = stackInfo->head.prev;
    }
}

u32 StackTraceAllocator::ActiveAllocs() {
    Lock lock{ mutex_ };

    u32 result{};
    auto ptr{ tail_ };
    while (ptr) {
        ++result;
        ptr = ptr->head.prev;
    }

    return result;
}

//
// Heap allocator.
////////////////////////////////////////////////

HeapAllocator::HeapAllocator() {
}

HeapAllocator::~HeapAllocator() {
}

void* HeapAllocator::Alloc(size_t size) {
    ++numAllocs;

    auto ptr{ malloc(size) };
    PROFILE_ALLOC(ptr, size);

    return ptr;
}

void* HeapAllocator::Realloc(void* memory, size_t size) {
    ++numAllocs;

    UGINE_ASSERT(size > 0);
    PROFILE_FREE(memory);
    auto ptr{ realloc(memory, size) };
    PROFILE_ALLOC(ptr, size);

    return ptr;
}

void HeapAllocator::Free(void* memory) {
    PROFILE_FREE(memory);
    free(memory);
}

void* HeapAllocator::AlignedAlloc(size_t size, size_t alignment) {
    ++numAllocs;

    auto ptr{ _aligned_malloc(size, alignment) };
    PROFILE_ALLOC(ptr, size);

    return ptr;
}

void* HeapAllocator::AlignedRealloc(void* memory, size_t size, size_t alignment) {
    ++numAllocs;

    UGINE_ASSERT(size > 0);
    PROFILE_FREE(memory);
    auto ptr{ _aligned_realloc(memory, size, alignment) };
    PROFILE_ALLOC(ptr, size);

    return ptr;
}

void HeapAllocator::AlignedFree(void* memory) {
    PROFILE_FREE(memory);
    _aligned_free(memory);
}

//
// Linear allocator.
////////////////////////////////////////////////

LinearAllocator::LinearAllocator(size_t size, IAllocator& allocator) {
    Init(size, allocator);
}

LinearAllocator::~LinearAllocator() {
    if (memory_) {
        allocator_->Free(memory_);
    }
}

void LinearAllocator::Init(size_t size, IAllocator& allocator) {
    allocator_ = &allocator;
    size_ = size;
    memory_ = allocator.Alloc(AlignTo(size, GetMemoryLayout().allocationGranularity));
}

void* LinearAllocator::Alloc(size_t size) {
    const auto position{ counter_.fetch_add(size) };

    UGINE_ASSERT(position < size_);

    return reinterpret_cast<char*>(memory_) + position;
}

void LinearAllocator::Free(void* memory) {
    // NOP.
}

void* LinearAllocator::Realloc(void* memory, size_t size) {
    if (memory == nullptr) {
        return Alloc(size);
    }

    UGINE_ASSERT(false && "Not supported!");
    UGINE_FATAL("Invalid operation");

    return nullptr;
}

void* LinearAllocator::AlignedAlloc(size_t size, size_t alignment) {
    // Nice, https://github.com/nem0/LumixEngine/blob/master/src/engine/allocators.cpp
    size_t position{};
    size_t end{ counter_ };
    for (;;) {
        position = AlignTo(end, alignment);

        if (counter_.compare_exchange_strong(end, position + size)) {
            break;
        }
    }

    UGINE_ASSERT(position < size_);

    return reinterpret_cast<char*>(memory_) + position;
}

void LinearAllocator::AlignedFree(void* memory) {
    // NOP.
}

void* LinearAllocator::AlignedRealloc(void* memory, size_t size, size_t alignment) {
    if (memory == nullptr) {
        return AlignedAlloc(size, alignment);
    }

    // Can't realloc, don't how big was original memory.
    UGINE_ASSERT(false && "Not supported!");
    UGINE_FATAL("Invalid operation");

    return nullptr;
}

void LinearAllocator::Reset() {
    counter_ = 0;
}

void LinearAllocator::Reset(Checkpoint chk) {
    counter_ = chk.Counter();
}

//
// IAllocator.
////////////////////////////////////////////////

MemoryLayout GetMemoryLayout() {
#ifdef WIN32
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);

    return MemoryLayout{
        .pageSize = sysInfo.dwPageSize,
        .allocationGranularity = sysInfo.dwAllocationGranularity,
    };
#else
    static_assert(false, "Not implemented");
#endif
}

IAllocator& IAllocator::Default() noexcept {
    //static HeapAllocator allocator;
    static MimallocAllocator allocator;

#ifdef UGINE_TRACE_ALLOCATIONS
    static StackTraceAllocator stAllocator{ allocator, true };
    return stAllocator;
#else // UGINE_TRACE_ALLOCATIONS
#ifdef UGINE_TRACE_ALLOCATIONS_CNT
    static CountedAllocator cntAllocator{ allocator };
    return cntAllocator;
#else  // UGINE_TRACE_ALLOCATIONS_CNT
    return allocator;
#endif // UGINE_TRACE_ALLOCATIONS_CNT
#endif // UGINE_TRACE_ALLOCATIONS
}

u32 IAllocator::NumAllocs() noexcept {
    return numAllocs;
}

void IAllocator::ResetCounter() noexcept {
    numAllocs = 0;
}

} // namespace ugine
