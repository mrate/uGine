#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <bitsery/adapter/buffer.h>
#include <bitsery/details/serialization_common.h>
#include <bitsery/traits/core/traits.h>

#include <ugine/Span.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

namespace std {
//UGINE_FORCE_INLINE ugine::String::ValueType* begin(ugine::String& s) noexcept {
//    return s.Begin();
//}
//UGINE_FORCE_INLINE ugine::String::ValueType* end(ugine::String& s) noexcept {
//    return s.End();
//}
//UGINE_FORCE_INLINE const ugine::String::ValueType* begin(const ugine::String& s) noexcept {
//    return s.Begin();
//}
//UGINE_FORCE_INLINE const ugine::String::ValueType* end(const ugine::String& s) noexcept {
//    return s.End();
//}
//UGINE_FORCE_INLINE const ugine::String::ValueType* cbegin(const ugine::String& s) noexcept {
//    return s.CBegin();
//}
//UGINE_FORCE_INLINE const ugine::String::ValueType* cend(const ugine::String& s) noexcept {
//    return s.CEnd();
//}
//
//template <typename T> T* begin(ugine::Vector<T>& v) noexcept {
//    return v.Begin();
//}
//
//template <typename T> T* end(ugine::Vector<T>& v) noexcept {
//    return v.End();
//}
//
//template <typename T> const T* begin(const ugine::Vector<T>& v) noexcept {
//    return v.Begin();
//}
//
//template <typename T> const T* end(const ugine::Vector<T>& v) noexcept {
//    return v.End();
//}

} // namespace std

namespace glm {
void serialize(auto& s, glm::vec4& vector) {
    s(vector.x);
    s(vector.y);
    s(vector.z);
    s(vector.w);
}
void serialize(auto& s, glm::vec3& vector) {
    s(vector.x);
    s(vector.y);
    s(vector.z);
}
void serialize(auto& s, glm::vec2& vector) {
    s(vector.x);
    s(vector.y);
}
void serialize(auto& s, glm::fquat& quat) {
    s(quat.x);
    s(quat.y);
    s(quat.z);
    s(quat.w);
}

void serialize(auto& s, std::pair<f32, glm::vec3>& vector) {
    s(vector.first);
    s(vector.second);
}

void serialize(auto& s, std::pair<f32, glm::fquat>& vector) {
    s(vector.first);
    s(vector.second);
}

void serialize(auto& s, glm::mat4& mat) {
    for (int i{}; i < 4; ++i) {
        s(mat[i]);
    }
}
} // namespace glm

namespace bitsery::traits {

// Buffer from ugine::Span
template <typename T> struct BufferAdapterTraits<ugine::Span<T>> {
    using TIterator = std::remove_cv_t<T>*;
    using TConstIterator = const std::remove_cv_t<T>*;
    using TValue = std::remove_cv_t<T>;
};

// Container traits for ugine::Span
template <typename T> struct ContainerTraits<ugine::Span<T>> {
    using TValue = std::remove_cv_t<T>;

    static constexpr bool isResizable = false;
    static constexpr bool isContiguous = true;
};

// Buffer from ugine::Vector
template <typename T> struct BufferAdapterTraits<ugine::Vector<T>> {
    using TIterator = std::remove_cv_t<T>*;
    using TConstIterator = const std::remove_cv_t<T>*;
    using TValue = std::remove_cv_t<T>;

    static void increaseBufferSize(ugine::Vector<T>& buffer, size_t currOffset, size_t newOffset) { buffer.Resize(newOffset); }
};

// Container traits for ugine::Vector
//template <typename T> struct ContainerTraits<ugine::Vector<T>> {
//    using TValue = std::remove_cv_t<T>;
//
//    static constexpr bool isResizable = true;
//    static constexpr bool isContiguous = true;
//
//    static void resize(ugine::Vector<T>& v, size_t size) { v.Resize(size); }
//    static size_t size(const ugine::Vector<T>& v) { return v.Size(); }
//};

// Base class for ugine containers.
template <typename T, bool Resizable, bool Contiguous> struct UgineContainer {
    using TValue = typename T::ValueType;
    static constexpr bool isResizable = Resizable;
    static constexpr bool isContiguous = Contiguous;
    static size_t size(const T& container) { return container.Size(); }
};

// Specialization for resizable ugine containers.
template <typename T, bool Contiguous> struct UgineContainer<T, true, Contiguous> {
    using TValue = typename T::ValueType;
    static constexpr bool isResizable = true;
    static constexpr bool isContiguous = Contiguous;
    static size_t size(const T& container) { return container.Size(); }
    static void resize(T& container, size_t size) { resizeImpl(container, size, std::is_default_constructible<TValue>{}); }

private:
    static void resizeImpl(T& container, size_t size, std::true_type) { container.Resize(size); }
    static void resizeImpl(T& container, size_t newSize, std::false_type) {
        const auto oldSize = size(container);
        for (auto it = oldSize; it < newSize; ++it) {
            container.PushBack(::bitsery::Access::create<TValue>());
        }
        if (oldSize > newSize) {
            container.Shrink(newSize);
        }
    }
};

template <typename T> struct ContainerTraits<ugine::Vector<T>> : public UgineContainer<ugine::Vector<T>, true, true> {};

template <> struct ContainerTraits<ugine::String> {
    using TValue = typename ugine::String::ValueType;
    static constexpr bool isResizable = true;
    static constexpr bool isContiguous = true;
    static size_t size(const ugine::String& container) { return container.Size(); }
    static void resize(ugine::String& container, size_t size) { container.Resize(size); }
};

template <> struct TextTraits<ugine::String> {
    using TValue = typename ugine::String::ValueType;
    // string is automatically null-terminated
    static constexpr bool addNUL = false;

    // is is not 100% accurate, but for performance reasons assume that string
    // stores text, not binary data
    static size_t length(const ugine::String& str) { return str.Size(); }
};

template <size_t N> struct TextTraits<ugine::StaticString<N>> {
    using TValue = char;
    static constexpr bool addNUL = true;

    static size_t length(const TValue (&container)[N]) { return std::char_traits<TValue>::length(container); }
};

} // namespace bitsery::traits

namespace std {
void serialize(auto& s, std::string& str) {
    s.container1b(str, 10000);
}
} // namespace std

namespace ugine {
using InputAdapter = bitsery::InputBufferAdapter<Span<const u8>>;
using OutputAdapter = bitsery::OutputBufferAdapter<Vector<u8>>;
} // namespace ugine
