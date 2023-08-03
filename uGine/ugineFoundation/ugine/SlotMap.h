#pragma once

#include <ugine/Memory.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <deque>

namespace ugine {

template <typename _ValueType, typename _KeyType = u64, size_t _PageSize = 4096> class SlotMap {
public:
    using InternalKeyType = u64;
    using KeyType = _KeyType;

    //using _T = typename std::enable_if_t<std::is_same_v<decltype(static_cast<InternalKeyType>(KeyType)), InternalKeyType>>;

    using ValueType = _ValueType;
    using GenerationType = u32;
    using IndexType = u32;

    static constexpr GenerationType MaxGeneration{ GenerationType(-1) };
    static constexpr IndexType MaxIndex{ IndexType(-1) };

    static constexpr size_t PageSize{ _PageSize };
    static constexpr size_t SlotSize{ sizeof(ValueType) };
    static constexpr size_t ValuesPerPage{ PageSize / SlotSize };

    static_assert(ValuesPerPage > 0);
    static_assert(PageSize >= SlotSize);

    struct Stats {
        u64 pageAllocations{};
        u64 pageDeallocations{};
        u64 overflows{};
    };

    explicit SlotMap(IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator }
        , pages_{ allocator }
        , indices_{ allocator } {}
    ~SlotMap() = default;

    // TODO: Allow copy for copyable inner types.
    SlotMap(const SlotMap&) = delete;
    SlotMap& operator=(const SlotMap&) = delete;

    template <typename... Args> KeyType Emplace(Args&&... args) {
        if (freeList_.empty()) {
            AddPage();
        }

        ++size_;
        const auto freeKey{ freeList_.front() };
        freeList_.pop_front();

        const auto [generation, index] = DecodeKey(freeKey);

        indices_[index] = freeKey;

        const auto pageIndex{ GetPageIndex(index) };

        auto& page{ pages_[pageIndex] };
        if (page.usedSlots == 0) {
            page.Allocate();
        }
        ++page.usedSlots;

        auto memory{ MemoryAt(index) };
        new (memory) ValueType(std::forward<Args&&>(args)...);

        return static_cast<KeyType>(freeKey);
    }

    ValueType* Get(KeyType key) const {
        const auto [generation, index] = DecodeKey(static_cast<InternalKeyType>(key));
        if (generation == 0) {
            return nullptr;
        }

        if (index >= indices_.Size()) {
            return nullptr;
        }

        const auto [exGeneration, exIndex] = DecodeKey(indices_[index]);

        if (exGeneration != generation) {
            return nullptr;
        }

        return MemoryAt(index);
    }

    void Erase(KeyType key) {
        const auto [generation, index] = DecodeKey(static_cast<InternalKeyType>(key));
        const auto [exGen, exIndex] = DecodeKey(indices_[index]);

        if (exGen != generation) {
            return;
        }

        --size_;
        const auto pageIndex{ GetPageIndex(index) };

        auto value{ MemoryAt(index) };
        value->~ValueType();

        auto& page{ pages_[pageIndex] };
        if (--page.usedSlots == 0) {
            page.Free();
        }

        if (generation < MaxGeneration) {
            indices_[index] = 0;
            freeList_.push_front(EncodeKey(generation + 1, index));
        } else {
            ++stats_.overflows;
        }
    }

    constexpr size_t Size() const { return size_; }
    constexpr size_t Capacity() const { return capacity_; }
    constexpr bool Empty() const { return size_ == 0; }

    void Clear() {
        for (auto& key : indices_) {
            const auto [generation, index] = DecodeKey(key);
            if (generation > 0) {
                auto value{ MemoryAt(index) };
                value->~ValueType();
            }
        }

        pages_.Clear();
        indices_.Clear();
        size_ = 0;
        capacity_ = 0;
        freeList_.clear();
    }

    const Stats& GetStats() const { return stats_; }

private:
    struct Page {
        IAllocator& allocator;
        void* rawMemory{};
        size_t rawMemorySize{};
        size_t usedSlots{};
        Stats& stats{};

        explicit Page(Stats& stats, IAllocator& allocator) noexcept
            : allocator{ allocator }
            , stats{ stats } {}

        Page(Page&& other) noexcept
            : allocator{ other.allocator }
            , stats{ other.stats } {
            std::swap(rawMemory, other.rawMemory);
            std::swap(rawMemorySize, other.rawMemorySize);
            std::swap(usedSlots, other.usedSlots);
        }

        Page& operator=(Page&& other) {
            stats = other.stats;
            std::swap(rawMemory, other.rawMemory);
            std::swap(rawMemorySize, other.rawMemorySize);
            std::swap(usedSlots, other.usedSlots);

            return *this;
        }

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;

        ~Page() { Free(); }

        void Allocate() {
            UGINE_ASSERT(rawMemorySize == 0);
            UGINE_ASSERT(usedSlots == 0);

            rawMemorySize = PageSize;
            rawMemory = allocator.AlignedAlloc(rawMemorySize, alignof(ValueType));

            ++stats.pageAllocations;
        }

        void Free() {
            if (rawMemorySize) {
                ++stats.pageDeallocations;

                allocator.AlignedFree(rawMemory);
                rawMemorySize = 0;
                usedSlots = 0;
            }
        }
    };

    constexpr InternalKeyType EncodeKey(GenerationType generation, IndexType index) const noexcept {
        return static_cast<InternalKeyType>(generation) << 32 | static_cast<InternalKeyType>(index);
    }

    constexpr std::pair<GenerationType, IndexType> DecodeKey(InternalKeyType key) const noexcept {
        const auto gen{ static_cast<GenerationType>((key >> 32) & MaxGeneration) };
        const auto idx{ static_cast<IndexType>(key & MaxIndex) };

        return std::make_pair(gen, idx);
    }

    ValueType* MemoryAt(IndexType index) const {
        const auto pageIndex{ GetPageIndex(index) };
        const auto pageRelativeIndex{ GetPageRelativeIndex(index) };

        auto& page{ pages_[pageIndex] };
        return reinterpret_cast<ValueType*>(&static_cast<u8*>(page.rawMemory)[pageRelativeIndex * SlotSize]);
    }

    static constexpr size_t GetPageIndex(IndexType index) noexcept { return index / ValuesPerPage; }

    static constexpr size_t GetPageRelativeIndex(IndexType index) noexcept {
        const auto pageIndex{ GetPageIndex(index) };
        return index - (pageIndex * ValuesPerPage);
    }

    void AddPage() {
        const IndexType startIndex{ static_cast<IndexType>(pages_.Size() * ValuesPerPage) };
        const IndexType endIndex{ startIndex + static_cast<u32>(ValuesPerPage) };

        for (auto begin{ startIndex }; begin < endIndex; ++begin) {
            freeList_.push_back(EncodeKey(1, begin));
        }

        pages_.EmplaceBack(stats_, allocator_);
        indices_.Resize(endIndex);
    }

    AllocatorRef allocator_;
    Vector<Page> pages_;
    Vector<InternalKeyType> indices_;
    std::deque<InternalKeyType> freeList_;

    size_t size_{};
    size_t capacity_{};

    Stats stats_{};
};

} // namespace ugine