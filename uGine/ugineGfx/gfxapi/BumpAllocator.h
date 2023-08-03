#pragma once

#include <gfxapi/Handle.h>
#include <ugine/Ugine.h>

namespace ugine::gfxapi {

struct GpuAllocation {
    BufferHandle buffer;
    void* mapped{};
    u64 offset{};
    u64 size{};

    template <typename T> T* As() {
        UGINE_ASSERT(sizeof(T) <= size);
        return reinterpret_cast<T*>(mapped);
    }
};

class BumpAllocator {
public:
    virtual ~BumpAllocator() = default;

    virtual GpuAllocation Allocate(size_t size, size_t alignment) = 0;
};

} // namespace ugine::gfxapi
