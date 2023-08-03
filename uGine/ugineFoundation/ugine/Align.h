#pragma once

namespace ugine {

template <typename T, typename U> constexpr size_t Align(const T& x) {
    return (sizeof(x) + sizeof(U) - 1) & ~(sizeof(U) - 1);
}

template <typename T> constexpr size_t AlignTo(size_t v) {
    return (sizeof(T) + v - 1) & ~(v - 1);
}

constexpr size_t AlignTo(size_t u, size_t v) {
    return (u + v - 1) & ~(v - 1);
}

inline u32 AlignBy(u32 offset, u32 size, u32 align) {
    return (offset % align == 0 || offset / align == (offset + size - 1) / align) ? offset : align * ((offset + align - 1) / align);
}

} // namespace ugine
