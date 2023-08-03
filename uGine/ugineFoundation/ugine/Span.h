#pragma once

#include <ugine/Ugine.h>

#include <array>
#include <type_traits>

namespace ugine {

template <typename T> class Span {
public:
    constexpr Span() noexcept = default;
    
    constexpr Span(T* value, size_t len) noexcept
        : begin_{ value }
        , end_{ value + len } {}

    constexpr Span(T* begin, T* end) noexcept
        : begin_{ begin }
        , end_{ end } {}
    
    template <size_t N>
    explicit constexpr Span(T (&value)[N]) noexcept
        : begin_{ value }
        , end_{ value + N } {}

    template <size_t N>
    explicit constexpr Span(std::array<T, N>& arr) noexcept
        : begin_{ arr.data() }
        , end_{ arr.data() + N } {}

    template <typename U, size_t N>
        requires std::is_convertible_v<const U, T>
    explicit constexpr Span(std::array<U, N>& arr) noexcept
        : begin_{ arr.data() }
        , end_{ arr.data() + N } {}

    template <typename U, size_t N>
        requires std::is_convertible_v<const U (*)[], T (*)[]>
    explicit constexpr Span(const std::array<U, N>& arr) noexcept
        : begin_{ static_cast<T*>(arr.data()) }
        , end_{ static_cast<T*>(arr.data() + N) } {}

    template <typename U>
        requires std::is_convertible_v<U (*)[], T (*)[]>
    explicit constexpr Span(Span<U> other)
        : begin_{ static_cast<T*>(other.Begin()) }
        , end_{ static_cast<T*>(other.End()) } {}

    constexpr size_t Size() const { return end_ - begin_; }
    constexpr bool Empty() const { return end_ == begin_; }

    operator Span<const T>() const { return Span<const T>{ begin_, end_ }; }

    T& operator[](size_t index) const {
        UGINE_ASSERT(index < Size());
        return *(begin_ + index);
    }

    constexpr T* Begin() const { return begin_; }
    constexpr T* End() const { return end_; }
    constexpr T* Data() const { return begin_; }

    friend constexpr T* begin(const Span& s) { return s.Begin(); }
    friend constexpr T* end(const Span& s) { return s.End(); }
    friend constexpr T* cbegin(const Span& s) { return s.Begin(); }
    friend constexpr T* cend(const Span& s) { return s.End(); }

private:
    T* begin_{};
    T* end_{};
};

} // namespace ugine