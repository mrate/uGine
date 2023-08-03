#pragma once

#include <ugine/Hash.h>
#include <ugine/Memory.h>
#include <ugine/Span.h>
#include <ugine/Ugine.h>

#include <array>
#include <string>
#include <type_traits>

namespace ugine {

class StringView;

class String final {
public:
    using ValueType = char;

    friend ValueType* begin(String& s) noexcept { return s.Begin(); }
    friend ValueType* end(String& s) noexcept { return s.End(); }
    friend const ValueType* begin(const String& s) noexcept { return s.Begin(); }
    friend const ValueType* end(const String& s) noexcept { return s.End(); }
    friend const ValueType* cbegin(const String& s) noexcept { return s.CBegin(); }
    friend const ValueType* cend(const String& s) noexcept { return s.CEnd(); }

    String(IAllocator& allocator = IAllocator::Default());
    String(const ValueType* src, IAllocator& allocator = IAllocator::Default());
    String& operator=(const ValueType* str);

    String(Span<const ValueType> src, IAllocator& allocator = IAllocator::Default());
    String& operator=(Span<const ValueType> str);

    String(const String& other);
    String& operator=(const String& other);

    String(String&& other) noexcept;
    String& operator=(String&& other);

    String(const StringView& other, IAllocator& allocator = IAllocator::Default());
    String& operator=(const StringView& other);

    ~String();

    UGINE_FORCE_INLINE const ValueType* Begin() const noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE const ValueType* End() const noexcept { return GetStorage() + size_; }
    UGINE_FORCE_INLINE const ValueType* CBegin() const noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE const ValueType* CEnd() const noexcept { return GetStorage() + size_; }
    UGINE_FORCE_INLINE ValueType* Begin() noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE ValueType* End() noexcept { return GetStorage() + size_; }

    UGINE_FORCE_INLINE const ValueType* begin() const noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE const ValueType* end() const noexcept { return GetStorage() + size_; }
    UGINE_FORCE_INLINE const ValueType* cbegin() const noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE const ValueType* cend() const noexcept { return GetStorage() + size_; }
    UGINE_FORCE_INLINE ValueType* begin() noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE ValueType* end() noexcept { return GetStorage() + size_; }

    UGINE_FORCE_INLINE ValueType* Data() noexcept { return GetStorage(); }
    UGINE_FORCE_INLINE const ValueType* Data() const noexcept { return GetStorage(); }

    UGINE_FORCE_INLINE size_t Size() const { return size_; }
    UGINE_FORCE_INLINE bool Empty() const { return size_ == 0; }

    void Clear() {
        *GetStorage() = '\0';
        size_ = 0;
    }

    void Resize(size_t newSize) {
        if (newSize == 0) {
            Clear();
        } else {
            if (newSize > size_) {
                Reserve(newSize);
                size_ = newSize;
            } else {
                size_ = newSize;
                GetStorage()[size_ + 1] = '\0';
            }
        }
    }

    void Append(const String& str);
    void Append(const ValueType* str);
    void Append(const StringView& str);

    String& operator+=(const String& str) {
        Append(str);
        return *this;
    }

    String& operator+=(const ValueType* str) {
        Append(str);
        return *this;
    }

    ValueType& operator[](size_t index) noexcept { return GetStorage()[index]; }
    ValueType operator[](size_t index) const noexcept { return GetStorage()[index]; }

    explicit operator Span<ValueType>() { return Span<ValueType>(Begin(), End()); }
    explicit operator Span<const ValueType>() const { return Span<const ValueType>(Begin(), End()); }

    Span<ValueType> MutableSpan() { return Span<ValueType>(Begin(), End()); }
    Span<const ValueType> ToSpan() { return Span<const ValueType>(Begin(), End()); }
    Span<const ValueType> ToSpan() const { return Span<const ValueType>(Begin(), End()); }
    Span<const ValueType> SubStr(size_t start, size_t len) const;

    //friend bool operator==(const String& a, const String& b);
    //friend bool operator==(const String& a, const ValueType* b);
    //friend bool operator!=(const String& a, const String& b) { return !(a == b); }
    //friend bool operator!=(const String& a, const ValueType* b) { return !(a == b); }

    friend bool operator<(const String& a, const String& b) { return strcmp(a.Data(), b.Data()) < 0; }

private:
    static constexpr auto SMALL_BUFFER_OPTIMIZATION{ 16 };

    void Init(const ValueType* src, size_t size);
    void Assign(const ValueType* src, size_t size);

    ValueType* GetStorage() noexcept { return capacity_ ? dynamic_ : static_; }
    const ValueType* GetStorage() const noexcept { return capacity_ ? dynamic_ : static_; }

    void Reserve(size_t size);

    AllocatorRef allocator_;
    union {
        ValueType* dynamic_;
        ValueType static_[SMALL_BUFFER_OPTIMIZATION];
    };
    size_t size_{};
    size_t capacity_{};
};

template <size_t N> class StaticString {
public:
    using ValueType = char;
    static constexpr auto SIZE{ N };

    StaticString() noexcept { Clear(); }
    explicit StaticString(const ValueType* str) noexcept { Append(str); }
    explicit StaticString(const StringView& str) noexcept;
    explicit StaticString(Span<ValueType> str) noexcept { Append(str.Data(), str.Size()); }

    StaticString& operator=(const ValueType* str) {
        Clear();
        Append(str);
        return *this;
    }
    StaticString& operator=(const StringView& str);
    StaticString& operator=(Span<ValueType> str) {
        Clear();
        Append(str.Data(), str.Size());
        return *this;
    }

    template <size_t M> explicit StaticString(const StaticString<M>& str) { Append(str); }
    template <size_t M> StaticString& operator=(const StaticString<M>& str) {
        Clear();
        Append(str);
        return *this;
    }

    ValueType& operator[](size_t index) {
        UGINE_ASSERT(index < size_);
        return data_[index];
    }

    ValueType operator[](size_t index) const {
        UGINE_ASSERT(index < size_);
        return data_[index];
    }

    size_t Size() const { return size_; }
    size_t Limit() const { return SIZE; }
    bool Empty() const { return size_ == 0; }
    void Resize(size_t newSize) {
        UGINE_ASSERT(newSize <= SIZE);
        size_ = newSize;
        data_[size_] = '\0';
    }

    void Clear() {
        size_ = 0;
        data_[0] = '\0';
    }

    ValueType* Data() { return data_.data(); }
    const ValueType* Data() const { return data_.data(); }

    ValueType* Begin() { return data_.data(); }
    ValueType* End() { return data_.data() + size_; }
    const ValueType* Begin() const { return data_.data(); }
    const ValueType* End() const { return data_.data() + size_; }
    const ValueType* CBegin() const { return data_.data(); }
    const ValueType* CEnd() const { return data_.data() + size_; }

    friend ValueType* begin(StaticString& str) { return str.Begin(); }
    friend ValueType* end(StaticString& str) { return str.End(); }
    friend const ValueType* begin(const StaticString& str) { return str.Begin(); }
    friend const ValueType* end(const StaticString& str) { return str.End(); }

    void Append(const ValueType* str) { Append(str, strlen(str)); }
    void Append(const StringView& str);
    void Append(char ch) { Append(&ch, 1); }
    void Append(Span<ValueType> str) { Append(str.Data(), str.Size()); }
    template <size_t M> void Append(const StaticString<M>& str) { Append(str.Data(), str.Size()); }

    //friend bool operator==(const StaticString& a, const ValueType* b) { return a.Equals(b); }
    //friend bool operator==(const StaticString& a, const String& b) { return a.Equals(b.Data(), b.Size()); }
    //friend bool operator==(const StaticString& a, Span<ValueType> b) { return a.Equals(b.Data(), b.Size()); }
    //friend bool operator!=(const StaticString& a, const ValueType* b) { return !(a == b); }
    //friend bool operator!=(const StaticString& a, const String& b) { return !(a == b); }
    //friend bool operator!=(const StaticString& a, Span<ValueType> b) { return !(a == b); }

private:
    void Append(const ValueType* str, size_t size) {
        const auto toAppend{ std::min(SIZE - size_, size) };
        memcpy(data_.data() + size_, str, toAppend);
        size_ += toAppend;
        data_[size_] = '\0';
    }

    bool Equals(const ValueType* str) const {
        auto data{ data_.data() };
        for (size_t i{}; i < size_; ++i, ++data, ++str) {
            if (*str == '\0' || *data != *str) {
                return false;
            }
        }
        return true;
    }

    bool Equals(const ValueType* str, size_t size) const {
        if (size == size_) {
            auto data{ data_.data() };
            for (size_t i{}; i < size_; ++i, ++data, ++str) {
                if (*data != *str) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    std::array<ValueType, N + 1> data_;
    size_t size_{};
};

class StringView {
public:
    constexpr StringView() noexcept = default;
    constexpr StringView(const char* value, size_t len) noexcept
        : begin_{ value }
        , end_{ value + len } {}
    constexpr StringView(const char* value) noexcept
        : begin_{ value }
        , end_{ value + std::char_traits<char>::length(value) } {}

    constexpr StringView(const char* begin, const char* end) noexcept
        : begin_{ begin }
        , end_{ end } {}

    template <size_t N>
    constexpr StringView(const char (&value)[N]) noexcept
        : begin_{ value }
        , end_{ value + N + 1 /* null terminator */ } {}

    template <size_t N>
    constexpr StringView(std::array<char, N>& arr) noexcept
        : begin_{ arr.data() }
        , end_{ arr.data() + N + 1 /* null terminator */ } {}

    StringView(const String& string) noexcept
        : begin_{ string.Data() }
        , end_{ string.Data() + string.Size() } {}

    template <size_t N>
    constexpr StringView(const StaticString<N>& string) noexcept
        : begin_{ string.Data() }
        , end_{ string.Data() + string.Size() } {}

    constexpr StringView(const std::string& str) noexcept
        : begin_{ str.data() }
        , end_{ str.data() + str.size() } {}

    //template <typename U, size_t N>
    //    requires std::is_convertible_v<const U, T>
    //explicit Span(std::array<U, N>& arr) noexcept
    //    : begin_{ arr.data() }
    //    , end_{ arr.data() + N } {}

    //template <typename U, size_t N>
    //    requires std::is_convertible_v<const U (*)[], T (*)[]>
    //explicit Span(const std::array<U, N>& arr) noexcept
    //    : begin_{ static_cast<T*>(arr.data()) }
    //    , end_{ static_cast<T*>(arr.data() + N) } {}

    //template <typename U>
    //    requires std::is_convertible_v<U (*)[], const char (*)[]>

    constexpr StringView(Span<const char> other)
        : begin_{ static_cast<const char*>(other.Begin()) }
        , end_{ static_cast<const char*>(other.End()) } {}

    constexpr size_t Size() const { return end_ - begin_; }
    constexpr bool Empty() const { return end_ == begin_; }

    const char operator[](size_t index) const {
        UGINE_ASSERT(index < Size());
        return *(begin_ + index);
    }

    constexpr const char* Begin() const { return begin_; }
    constexpr const char* End() const { return end_; }
    constexpr const char* Data() const { return begin_; }

    friend constexpr const char* begin(const StringView& s) { return s.Begin(); }
    friend constexpr const char* end(const StringView& s) { return s.End(); }
    friend constexpr const char* cbegin(const StringView& s) { return s.Begin(); }
    friend constexpr const char* cend(const StringView& s) { return s.End(); }

    constexpr Span<const char> ToSpan() const { return Span<const char>{ begin_, end_ }; }

private:
    const char* begin_{};
    const char* end_{};
};

template <size_t N> StaticString<N>::StaticString(const StringView& str) noexcept {
    Append(str.Data(), str.Size());
}

template <size_t N> StaticString<N>& StaticString<N>::operator=(const StringView& str) {
    Clear();
    Append(str.Data(), str.Size());
    return *this;
}

template <size_t N> void StaticString<N>::Append(const StringView& str) {
    Append(str.Data(), str.Size());
}

bool operator==(StringView a, StringView b);

inline bool operator!=(StringView a, StringView b) {
    return !(a == b);
}

//inline bool operator==(const String& a, const StringView& b) {
//    return StringView{ a } == b;
//}

//inline bool operator!=(const String& a, const StringView& b) {
//    return !(a == b);
//}

//inline bool operator==(const StringView& a, const String& b) {
//    return a == StringView{ b };
//}
//
//inline bool operator!=(const StringView& a, const String& b) {
//    return !(a == b);
//}

//template <size_t N> inline bool operator==(const StaticString<N>& a, const StringView& b) {
//    return StringView{ a } == b;
//}
//
//template <size_t N> inline bool operator!=(const StaticString<N>& a, const StringView& b) {
//    return !(a == b);
//}
//
//template <size_t N> inline bool operator==(const StringView& a, const StaticString<N>& b) {
//    return a == StringView{ b };
//}
//
//template <size_t N> inline bool operator!=(const StringView& a, const StaticString<N>& b) {
//    return !(a == b);
//}

class StringID final {
public:
    using HashType = u64;

    StringID() noexcept {}
    explicit StringID(HashType hash) noexcept
        : hash_{ hash } {}
    constexpr StringID(const StringID& other) noexcept
        : hash_{ other.hash_ } {}
    explicit constexpr StringID(StringView str) noexcept
        : hash_{ fnv1a(str.Data(), str.Size()) } {}

    constexpr HashType Value() const { return hash_; }
    constexpr operator HashType() const { return hash_; }

    constexpr bool operator==(const StringID& other) { return hash_ == other.hash_; }
    constexpr bool operator!=(const StringID& other) { return hash_ != other.hash_; }
    constexpr bool operator<(const StringID& other) { return hash_ < other.hash_; }

private:
    HashType hash_{};
};

UGINE_FORCE_INLINE constexpr StringID operator""_hs(const char* str, size_t len) {
    return StringID{ StringView{ str, len } };
}

} // namespace ugine

namespace std {
template <> struct hash<ugine::StringID> {
    size_t operator()(const ugine::StringID& str) const noexcept { return size_t(str.Value()); }
};
} // namespace std
