#pragma once

#include <type_traits>

namespace ugine {

///
/// Helper struct for creating unique aliases from same types.
/// Use UGINE_UTILS_UNIQUE_TYPE(...) macro:
///
///     UGINE_UTILS_UNIQUE_TYPE(Key1, u64);
///     UGINE_UTILS_UNIQUE_TYPE(Key2, u64);
///
/// This will create two distinct types Key1 and Key2, which are *explicitly* convertible to u64,
///  but behave as a distinct types, so following code won't compile:
///
///     Key1 key1 { 1 };
///     Key2 key2 { 1 };
///
///     // Won't compile (different types):
///     assert(key1 == key2);
///
template <typename T, typename U> struct UniqueType {
    using Type = T;
    using ValueType = U;

    UniqueType() = default;
    explicit UniqueType(ValueType value)
        : m_value{ value } {}

    explicit operator ValueType() const { return m_value; }

    //template <typename _T = ValueType> explicit operator typename std::enable_if_t<std::is_convertible_v<_T, bool>>() const {
    //    return static_cast<bool>(m_value);
    //}

    explicit operator bool() const { return static_cast<bool>(m_value); }
    friend bool operator==(const Type& a, const Type& b) { return a.m_value == b.m_value; }
    friend bool operator!=(const Type& a, const Type& b) { return !(a == b); }
    friend bool operator<(Type a, Type b) { return a.m_value < b.m_value; }
    friend bool operator>(Type a, Type b) { return a.m_value > b.m_value; }

    ValueType m_value{};
};

} // namespace ugine

#define UGINE_UTILS_UNIQUE_TYPE(T, U)                                                                                                                          \
    struct T final : public ugine::UniqueType<T, U> {                                                                                                          \
        using UniqueType::UniqueType;                                                                                                                          \
    };