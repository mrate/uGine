#pragma once

#include <ugine/Ugine.h>

#include <streambuf>

#define UGINE_MOVABLE(T)                                                                                                                                       \
    T(T&&) = default;                                                                                                                                          \
    T& operator=(T&&) = default;

#define UGINE_NO_COPY(T)                                                                                                                                       \
    T(const T&) = delete;                                                                                                                                      \
    T& operator=(const T&) = delete;

#define UGINE_MOVABLE_ONLY(T)                                                                                                                                  \
    UGINE_NO_COPY(T)                                                                                                                                           \
    UGINE_MOVABLE(T)

#define UGINE_FLAGS(T, UT)                                                                                                                                     \
    inline T operator|(T a, T b) { return static_cast<T>(static_cast<UT>(a) | static_cast<UT>(b)); }                                                           \
    inline UT operator&(T a, T b) { return (static_cast<UT>(a) & static_cast<UT>(b)); }                                                                        \
    inline T& operator&=(T& a, T b) {                                                                                                                          \
        a = static_cast<T>(a & b);                                                                                                                             \
        return a;                                                                                                                                              \
    }                                                                                                                                                          \
    inline T& operator|=(T& a, T b) {                                                                                                                          \
        a = a | b;                                                                                                                                             \
        return a;                                                                                                                                              \
    }                                                                                                                                                          \
    inline T operator~(T a) { return static_cast<T>(~static_cast<UT>(a)); }                                                                                    \
    inline bool operator==(T& a, UT b) { return static_cast<UT>(a) == b; }                                                                                     \
    inline bool operator!=(T& a, UT b) { return !(a == b); }

#define UGINE_BIT(n) (1ull << (n))

namespace ugine {

template <typename T> UGINE_FORCE_INLINE constexpr u8 BitCount(T n) {
    u8 counter{};
    while (n) {
        ++counter;
        n &= (n - 1);
    }
    return counter;
}

template <typename T> class ScopedValue {
public:
    ScopedValue(T& t, T val)
        : val_{ t }
        , ref_{ t } {
        ref_ = val;
    }

    ~ScopedValue() { ref_ = val_; }

private:
    T& ref_;
    T val_;
};

class MemoryStreamBuffer final : public std::streambuf {
public:
    MemoryStreamBuffer(const void* startAddress, size_t sizeBytes) {
        auto start{ static_cast<char*>(const_cast<void*>(startAddress)) };
        setg(start, start, start + sizeBytes);
    }
};

template <bool, typename T> struct ArgTypeBase {};
template <typename T> struct ArgTypeBase<true, T> {
    using Type = T;
};
template <typename T> struct ArgTypeBase<false, T> {
    using Type = const T&;
};

template <typename T> using ArgType = ArgTypeBase<std::is_fundamental_v<T> || std::is_enum_v<T>, T>::Type;

} // namespace ugine
