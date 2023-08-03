#pragma once

#include <ugine/Vector.h>

#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>

namespace ugine {

template <class T> class ArrayProxy {
public:
    using ElementType = T;
    static constexpr auto ElementSize = sizeof(T);

    ArrayProxy() = default;

    ArrayProxy(const ArrayProxy&) = default;
    ArrayProxy& operator=(const ArrayProxy&) = default;

    ArrayProxy(ArrayProxy&& other) = default;
    ArrayProxy& operator=(ArrayProxy&& other) = default;

    template <size_t N>
    constexpr ArrayProxy(const T (&data)[N])
        : m_data{ data }
        , m_size{ N } {}

    constexpr ArrayProxy(const T* data, size_t size)
        : m_data{ data }
        , m_size{ size } {}

    template <typename _Alloc>
    constexpr ArrayProxy(const std::vector<T, _Alloc>& data)
        : m_data{ data.data() }
        , m_size{ data.size() } {}

    constexpr ArrayProxy(const Vector<T>& data)
        : m_data{ data.Data() }
        , m_size{ data.Size() } {}

    template <size_t N>
    constexpr ArrayProxy(const std::array<T, N>& arr)
        : m_data{ arr.data() }
        , m_size{ arr.size() } {}

    //template <typename T> constexpr ArrayProxy(const std::initializer_list<T>& list)
    //    : m_data{ list.begin() }
    //    , m_size{ list.size() }
    //{}

    //TODO: Disallow ArrayProxy<int> singleInvalid{ 5 };
    constexpr ArrayProxy(const T& data)
        : m_data{ &data }
        , m_size{ 1 } {}

    constexpr const T* data() const { return m_data; }

    constexpr std::size_t size() const { return m_size; }

    constexpr bool empty() const { return m_size == 0; }

    constexpr const T& operator[](size_t index) const { return m_data[index]; }

    template <typename N, typename F> std::vector<N> Convert(F&& f) const {
        if (empty()) {
            return {};
        } else {
            std::vector<N> res(m_size);
            std::transform(begin(), end(), std::begin(res), f);
            return res;
        }
    }

    template <typename N> std::vector<N> Convert() const {
        if (empty()) {
            return {};
        } else {
            std::vector<N> res(m_size);
            std::transform(begin(), end(), std::begin(res), [](auto&& t) { return static_cast<N>(t); });
            return res;
        }
    }

    constexpr const T* begin() const { return m_data; }

    constexpr const T* end() const { return m_data + m_size; }

    constexpr const T* cbegin() const { return m_data; }

    constexpr const T* cend() const { return m_data + m_size; }

private:
    const T* m_data{};
    std::size_t m_size{};
};

namespace __test {

    // Compile-time tests.
    constexpr bool TestArrayProxy() {
        const int arr[] = { 1, 2, 3 };
        ArrayProxy<int> arrArrayProxy{ arr };
        if (arrArrayProxy.size() != 3) {
            return false;
        }
        if (arrArrayProxy[0] != 1 || arrArrayProxy[2] != 3) {
            return false;
        }

        const std::array<int, 4> stdArr{ 1, 2, 3, 4 };
        ArrayProxy<int> stdArrArrayProxy{ stdArr };
        if (stdArrArrayProxy.size() != 4) {
            return false;
        }
        if (stdArrArrayProxy[1] != 2 || stdArrArrayProxy[3] != 4) {
            return false;
        }

        ArrayProxy<int> emptyArrayProxy{};
        if (!emptyArrayProxy.empty()) {
            return false;
        }
        for (auto v : emptyArrayProxy) {
            (void)v;
            return false;
        }

        int val{ 5 };
        ArrayProxy<int> single{ val };
        if (single.size() != 1) {
            return false;
        }
        if (single[0] != val) {
            return false;
        }

        // TODO: Disallow.
        //ArrayProxy<int> singleInvalid{ 5 };

        return true;
    }

    static_assert(TestArrayProxy(), "ArrayProxy test failed");
} // namespace __test

} // namespace ugine