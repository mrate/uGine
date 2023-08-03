#pragma once

#include <ugine/Ugine.h>
#include <ugine/UniqueType.h>

#include <optional>

namespace ugine::gfxapi {

class Device;

template <typename _Handle> class HandleDeleter {
public:
    using HandleType = _Handle;

    explicit HandleDeleter(Device* device)
        : m_device{ device } {}

    void Delete(HandleType handle);

    HandleDeleter(const HandleDeleter&) = delete;
    HandleDeleter& operator=(const HandleDeleter&) = delete;

    HandleDeleter(HandleDeleter&&) = default;
    HandleDeleter& operator=(HandleDeleter&&) = default;

private:
    Device* m_device{};
};

template <typename _Handle> class UniqueHandle {
public:
    using HandleType = _Handle;

    UniqueHandle() = default;

    UniqueHandle(HandleType handle, Device* device)
        : m_handle{ handle }
        , m_deleter{ HandleDeleter<HandleType>(device) } {}

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept {
        std::swap(m_handle, other.m_handle);
        std::swap(m_deleter, other.m_deleter);
    }

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        std::swap(m_handle, other.m_handle);
        std::swap(m_deleter, other.m_deleter);

        return *this;
    }

    ~UniqueHandle() {
        if (m_handle) {
            m_deleter->Delete(m_handle);
        }
    }

    explicit operator bool() const { return static_cast<bool>(m_handle); }

    explicit operator HandleType() const { return m_handle; }

    HandleType operator*() const { return m_handle; }

private:
    HandleType m_handle{};
    std::optional<HandleDeleter<HandleType>> m_deleter{};
};

#define CREATE_HANDLE(NAME, TYPE)                                                                                                                              \
    UGINE_UTILS_UNIQUE_TYPE(NAME, TYPE);                                                                                                                       \
    using NAME##Unique = UniqueHandle<NAME>;

CREATE_HANDLE(BufferHandle, u64);
CREATE_HANDLE(SamplerHandle, u64);
CREATE_HANDLE(GraphicsPipelineHandle, u64);
CREATE_HANDLE(ComputePipelineHandle, u64);
CREATE_HANDLE(TextureHandle, u64);
CREATE_HANDLE(RenderPassHandle, u64);
CREATE_HANDLE(FramebufferHandle, u64);
CREATE_HANDLE(FenceHandle, u64);
CREATE_HANDLE(SemaphoreHandle, u64);
CREATE_HANDLE(BindingHandle, u64);
CREATE_HANDLE(QueryPoolHandle, u64);

#undef CREATE_HANDLE

} // namespace ugine::gfxapi