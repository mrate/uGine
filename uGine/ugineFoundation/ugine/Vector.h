#pragma once

#include <ugine/Memory.h>
#include <ugine/Span.h>
#include <ugine/Ugine.h>
#include <ugine/Utils.h>

#include <type_traits>

namespace ugine {

namespace detail {

    template <typename T> class DynamicStorage {
    public:
        DynamicStorage() = default;
        DynamicStorage(const DynamicStorage&) = delete;
        DynamicStorage(DynamicStorage&& other) noexcept
            : data_{ std::exchange(other.data_, nullptr) }
            , capacity_{ std::exchange(other.capacity_, 0) } {}

        DynamicStorage& operator=(DynamicStorage&& other) noexcept {
            data_ = std::exchange(other.data_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
            return *this;
        }

        DynamicStorage Clone(IAllocator& allocator) const {
            DynamicStorage storage{};
            storage.data_ = reinterpret_cast<T*>(allocator.AlignedAlloc(capacity_ * sizeof(T), alignof(T)));
            storage.capacity_ = capacity_;
            return storage;
        }

        UGINE_FORCE_INLINE T* Data() { return data_; }
        UGINE_FORCE_INLINE const T* Data() const { return data_; }
        UGINE_FORCE_INLINE size_t Capacity() const { return capacity_; }

        void Swap(DynamicStorage& other) {
            std::swap(data_, other.data_);
            std::swap(capacity_, other.capacity_);
        }

        template <typename F> void Reserve(IAllocator& allocator, size_t capacity, F move) {
            if (capacity <= capacity_) {
                return;
            }

            if constexpr (std::is_trivially_copyable_v<T>) {
                data_ = reinterpret_cast<T*>(allocator.AlignedRealloc(data_, capacity * sizeof(T), alignof(T)));
            } else {
                auto newData{ static_cast<T*>(allocator.AlignedAlloc(capacity * sizeof(T), alignof(T))) };
                move(newData, data_);
                allocator.AlignedFree(data_);
                data_ = newData;
            }

            capacity_ = capacity;
        }

        void Destroy(IAllocator& allocator) {
            if (data_) {
                allocator.AlignedFree(data_);
                data_ = nullptr;
            }
            capacity_ = 0;
        }

    private:
        T* data_{};
        size_t capacity_{};
    };

    template <size_t N, typename T> class HybridStorage {
    public:
        static constexpr auto StaticSize{ sizeof(T) * N };

        HybridStorage() = default;
        HybridStorage(const HybridStorage&) = delete;
        HybridStorage(HybridStorage&& other)
            : dynamicSize_{ other.dynamicSize_ } {
            if (other.dynamicSize_) {
                dynamic_ = other.dynamic_;
                other.dynamicSize_ = 0;
            } else {
                memcpy(static_, other.static_, StaticSize);
            }
        }

        // TODO: Swap

        UGINE_FORCE_INLINE T* Data() { return dynamicSize_ ? dynamic_ : reinterpret_cast<T*>(static_); }
        UGINE_FORCE_INLINE const T* Data() const { return dynamicSize_ ? dynamic_ : reinterpret_cast<const T*>(static_); }
        UGINE_FORCE_INLINE size_t Capacity() const { return dynamicSize_ ? dynamicSize_ : StaticSize; }

        template <typename F> void Reserve(IAllocator& allocator, size_t capacity, F move) {
            if (capacity <= StaticSize || capacity <= dynamicSize_) {
                return;
            }

            if constexpr (std::is_trivially_copyable_v<T>) {
                if (dynamicSize_ == 0) {
                    auto ptr{ reinterpret_cast<T*>(allocator.AlignedAlloc(capacity * sizeof(T), alignof(T))) };
                    memcpy(ptr, static_, StaticSize);
                    dynamic_ = ptr;
                } else {
                    dynamic_ = reinterpret_cast<T*>(allocator.AlignedRealloc(dynamic_, capacity * sizeof(T), alignof(T)));
                }
            } else {
                auto newData{ static_cast<T*>(allocator.AlignedAlloc(capacity * sizeof(T), alignof(T))) };
                move(newData, Data());
                if (dynamicSize_) {
                    allocator.AlignedFree(dynamic_);
                }
                dynamic_ = newData;
            }

            dynamicSize_ = capacity;
        }

        void Destroy(IAllocator& allocator) {
            if (dynamicSize_) {
                allocator.AlignedFree(dynamic_);
                dynamicSize_ = 0;
            }
        }

    private:
        union {
            T* dynamic_{};
            std::aligned_storage_t<sizeof(T), alignof(T)> static_[StaticSize];
        };

        size_t dynamicSize_{}; // = 0 means static, otherwise dynamic
    };

} // namespace detail

template <typename T, typename _Storage> class BaseVector final {
public:
    using ValueType = T;
    using RefType = T&;
    using PtrType = T*;

    friend ValueType* begin(BaseVector& v) { return v.Begin(); }
    friend ValueType* end(BaseVector& v) { return v.End(); }
    friend const ValueType* begin(const BaseVector& v) { return v.Begin(); }
    friend const ValueType* end(const BaseVector& v) { return v.End(); }
    friend const ValueType* cbegin(const BaseVector& v) { return v.Begin(); }
    friend const ValueType* cend(const BaseVector& v) { return v.End(); }

    BaseVector(IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {}

    explicit BaseVector(size_t size, IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {
        if (size > 0) {
            Resize(size);
        }
    }

    BaseVector(const ValueType* values, size_t size, IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {
        Initialize(values, size);
    }

    BaseVector(size_t size, const ValueType& initialValue, IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {
        InitializeSingle(initialValue, size);
    }

    BaseVector(Span<const ValueType> span, IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {
        if (!span.Empty()) {
            Initialize(span.Data(), span.Size());
        }
    }

    BaseVector(std::initializer_list<ValueType> data, IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {
        Initialize(data.begin(), data.size());
    }

    BaseVector(const BaseVector& other)
        : allocator_{ other.allocator_ }
        , size_{ other.size_ }
        , storage_{ other.storage_.Clone(other.allocator_) } {
        for (u32 i{}; i < size_; ++i) {
            new (&Data()[i]) T{ other[i] };
        }
    }

    ~BaseVector() { Destroy(); }

    BaseVector(BaseVector&& other) noexcept
        : allocator_{ other.allocator_ }
        , storage_{ std::move(other.storage_) }
        , size_{ std::exchange(other.size_, 0) } {}

    BaseVector& operator=(BaseVector&& other) noexcept {
        Destroy();

        allocator_ = other.allocator_;
        storage_ = std::move(other.storage_);
        size_ = std::exchange(other.size_, 0);
        return *this;
    }

    BaseVector Clone() const { return BaseVector{ Data(), Size(), allocator_ }; }

    void Swap(BaseVector& other) noexcept {
        std::swap(allocator_, other.allocator_);
        std::swap(size_, other.size_);
        storage_.Swap(other.storage_);
    }

    UGINE_FORCE_INLINE size_t Size() const noexcept { return size_; }
    UGINE_FORCE_INLINE size_t Capacity() const noexcept { return storage_.Capacity(); }
    UGINE_FORCE_INLINE size_t DataSize() const noexcept { return sizeof(T) * size_; }

    UGINE_FORCE_INLINE ValueType& operator[](size_t index) noexcept { return Data()[index]; }
    UGINE_FORCE_INLINE const ValueType& operator[](size_t index) const noexcept { return Data()[index]; }

    UGINE_FORCE_INLINE explicit operator Span<ValueType>() { return Span<ValueType>{ Data(), size_ }; }
    UGINE_FORCE_INLINE explicit operator Span<const ValueType>() const { return Span<const ValueType>{ Data(), size_ }; }
    UGINE_FORCE_INLINE Span<const ValueType> ToSpan() const { return Span<const ValueType>{ Data(), size_ }; }
    UGINE_FORCE_INLINE Span<ValueType> ToSpan() { return Span<ValueType>{ Data(), size_ }; }

    UGINE_FORCE_INLINE ValueType* Begin() noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE ValueType* End() noexcept { return storage_.Data() + size_; }
    UGINE_FORCE_INLINE const ValueType* Begin() const noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE const ValueType* End() const noexcept { return storage_.Data() + size_; }
    UGINE_FORCE_INLINE const ValueType* CBegin() const noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE const ValueType* CEnd() const noexcept { return storage_.Data() + size_; }

    UGINE_FORCE_INLINE ValueType* begin() noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE ValueType* end() noexcept { return storage_.Data() + size_; }
    UGINE_FORCE_INLINE const ValueType* begin() const noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE const ValueType* end() const noexcept { return storage_.Data() + size_; }
    UGINE_FORCE_INLINE const ValueType* cbegin() const noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE const ValueType* cend() const noexcept { return storage_.Data() + size_; }

    UGINE_FORCE_INLINE ValueType* Data() noexcept { return storage_.Data(); }
    UGINE_FORCE_INLINE const ValueType* Data() const noexcept { return storage_.Data(); }

    UGINE_FORCE_INLINE bool Empty() const { return size_ == 0; }

    void Clear() {
        if (size_ > 0) {
            Destruct(Begin(), End());
            size_ = 0;
        }
    }

    void Resize(size_t size) {
        Reserve(size);

        for (size_t i{ size_ }; i < size; ++i) {
            new (&Data()[i]) ValueType{};
        }

        if (size < size_) {
            Destruct(&Data()[size], &Data()[size_]);
        }

        size_ = size;
    }

    void Shrink(size_t size) {
        UGINE_ASSERT(size <= size_);

        if (size < size_) {
            Destruct(&Data()[size], &Data()[size_]);
            size_ = size;
        }
    }

    UGINE_FORCE_INLINE void Reserve(size_t capacity) {
        storage_.Reserve(allocator_, capacity, [this](auto newData, auto data) { Move(newData, data, size_); });
    }

    void PushBack(const ValueType& t) {
        if (size_ == storage_.Capacity()) {
            Expand();
        }
        new (&Data()[size_]) ValueType{ std::move(t) };
        ++size_;
    }

    void PushBack(ValueType&& t) {
        if (size_ == storage_.Capacity()) {
            Expand();
        }
        new (&Data()[size_]) ValueType{ std::move(t) };
        ++size_;
    }

    template <typename... Args> void EmplaceBack(Args&&... args) {
        if (size_ == storage_.Capacity()) {
            Expand();
        }
        new (&Data()[size_]) ValueType{ std::forward<Args>(args)... };
        ++size_;
    }

    void PopBack() {
        if (size_ > 0) {
            Data()[size_].~ValueType();
            --size_;
        }
    }

    T PopFront() {
        UGINE_ASSERT(size_ > 0);

        ValueType res{ std::move(Data()[0]) };
        EraseAt(0);
        return std::move(res);
    }

    void EraseFront(size_t count) {
        if (count == 0) {
            return;
        }

        UGINE_ASSERT(count <= size_);

        if (count == size_) {
            Clear();
            return;
        }

        for (size_t i{}; i < count; ++i) {
            Data()[i].~ValueType();
        }

        if constexpr (std::is_trivially_copyable_v<T>) {
            memmove(Data(), Data() + count, sizeof(T) * (size_ - count));
        } else {
            for (size_t i{ count }; i < size_ - 1; ++i) {
                new (&Data()[i]) T{ std::move(Data()[i + 1]) };
                Data()[i + 1].~T();
            }
        }
        size_ -= count;
    }

    void EraseBack(size_t count) {
        if (count == 0) {
            return;
        }

        UGINE_ASSERT(count <= size_);

        Destruct(Begin() + size_ - count - 1, End());
        size_ -= count;
    }

    void EraseAt(size_t index) {
        UGINE_ASSERT(index < size_);

        Data()[index].~ValueType();
        if (index < size_ - 1) {
            if constexpr (std::is_trivially_copyable_v<ValueType>) {
                memmove(Data() + index, Data() + index + 1, sizeof(ValueType) * (size_ - index - 1));
            } else {
                for (size_t i{ index }; i < size_ - 1; ++i) {
                    new (&Data()[i]) ValueType{ std::move(Data()[i + 1]) };
                    Data()[i + 1].~T();
                }
            }
        }
        --size_;
    }

    template <typename P> void EraseIf(P pred) {
        for (size_t i{ size_ }; i > 0; --i) {
            if (pred(Data()[i - 1])) {
                EraseAt(i - 1);
            }
        }
    }

    void Erase(const ValueType& t) {
        const auto index{ IndexOf(t) };
        if (index >= 0) {
            EraseAt(index);
        }
    }

    void EraseReorder(const ValueType& t) { EraseReorderAt(IndexOf(t)); }

    void EraseReorderAt(int index) {
        if (size_ > 1) {
            std::swap(Data()[index], Back());
        }
        PopBack();
    }

    template <typename P> void EraseIfReorder(P pred) {
        if (size_ > 0) {
            auto back{ size_ - 1 };

            for (size_t i{}; i <= back; ++i) {
                if (pred(Data()[i])) {
                    std::swap(Data()[i], Data()[back]);
                    --back;
                }
            }

            size_ = back + 1;
            Destruct(&Data()[back], End());
        }
    }

    template <typename P> int FindIf(P pred) const {
        int index{};
        for (auto it{ Begin() }, end{ End() }; it != end; ++it, ++index) {
            if (pred(*it)) {
                return index;
            }
        }
        return -1;
    }

    int IndexOf(const ValueType& t) const {
        for (int i{}; i < size_; ++i) {
            if (Data()[i] == t) {
                return i;
            }
        }
        return -1;
    }

    UGINE_FORCE_INLINE ValueType& Back() {
        UGINE_ASSERT(size_ > 0);
        return Data()[size_ - 1];
    }

    UGINE_FORCE_INLINE const ValueType& Back() const {
        UGINE_ASSERT(size_ > 0);
        return Data()[size_ - 1];
    }

    UGINE_FORCE_INLINE ValueType& Front() {
        UGINE_ASSERT(size_ > 0);
        return Data()[0];
    }

    UGINE_FORCE_INLINE const ValueType& Front() const {
        UGINE_ASSERT(size_ > 0);
        return Data()[0];
    }

    template <typename U> UGINE_FORCE_INLINE void Append(const BaseVector<ValueType, U>& v) { Append(v.Data(), v.Size()); }
    UGINE_FORCE_INLINE void Append(Span<ValueType> s) { Append(s.Data(), s.Size()); }

    void Append(const ValueType* data, size_t size) {
        if (size > 0) {
            Reserve(Size() + size);

            if constexpr (std::is_trivially_copyable_v<T>) {
                memcpy(Data() + size_, data, sizeof(T) * size);
            } else {
                auto ptr{ &Data()[size_] };
                for (size_t i{}; i < size; ++i) {
                    new (ptr++) T{ data++ };
                }
            }

            size_ += size;
        }
    }

    // Algorithms.
    template <typename F> void ForEach(F f) {
        for (auto& v : *this) {
            f(v);
        }
    }

    template <typename F> void ForEach(F f) const {
        for (const auto& v : *this) {
            f(v);
        }
    }

private:
    using StorageType = _Storage;

    void Initialize(const ValueType* values, size_t size) {
        if (size > 0) {
            Reserve(size);
            size_ = size;
            for (size_t i{}; i < size; ++i) {
                new (&storage_.Data()[i]) T{ values[i] };
            }
        }
    }

    void InitializeSingle(const ValueType& value, size_t size) {
        if (size > 0) {
            Reserve(size);
            size_ = size;
            for (size_t i{}; i < size; ++i) {
                new (&storage_.Data()[i]) T{ value };
            }
        }
    }

    void Move(ValueType* dst, ValueType* src, size_t count) {
        for (size_t i{}; i < count; ++i) {
            new (&dst[i]) T{ std::move(src[i]) };
            src[i].~T();
        }
    }

    UGINE_FORCE_INLINE void Expand() { Reserve(storage_.Capacity() < 4 ? 4 : storage_.Capacity() * 2); }

    void Destruct(ValueType* begin, ValueType* end) {
        UGINE_ASSERT(begin <= end);

        for (; begin != end; ++begin) {
            begin->~T();
        }
    }

    UGINE_FORCE_INLINE void Destroy() {
        Clear();
        storage_.Destroy(allocator_);
    }

    AllocatorRef allocator_;
    size_t size_{};
    StorageType storage_;
};

template <typename T> using Vector = BaseVector<T, detail::DynamicStorage<T>>;
template <typename T, size_t N = 8> using HybridVector = BaseVector<T, detail::HybridStorage<N, T>>;

} // namespace ugine
