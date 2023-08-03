#pragma once

#include "Types.h"

namespace ugine::gfxapi {

class Swapchain {
public:
    virtual ~Swapchain() = default;

    virtual bool Present() = 0;
    virtual void Reset() = 0;

    virtual Format GetFormat() const = 0;
    virtual Extent2D GetExtent() const = 0;
    virtual TextureHandle GetTexture(u32 index) = 0;
    virtual SemaphoreHandle GetWaitSemaphore() { return {}; }
    virtual SemaphoreHandle GetSignalSemaphore() { return {}; }
    virtual FenceHandle GetFence() { return {}; }
    virtual u32 GetIndex() const = 0;
    virtual u32 GetCount() const = 0;
};

} // namespace ugine::gfxapi