#pragma once

#include <ugine/Ugine.h>

#include <array>

namespace ugine {

template <typename T> struct Delegate;

template <class Ret, class... Args> struct Delegate<Ret(Args...)> {
    //static constexpr auto StorageSize{ 128 };
    static constexpr auto StorageSize{ sizeof(void*) };
    //using Storage = std::aligned_storage<StorageSize, alignof(void*)>;

    using Storage = std::array<std::byte, StorageSize>;

    using Signature = Ret(Storage&, Args...);

    Delegate() = default;

    Delegate(const Delegate& other) noexcept
        : function_{ other.function_ }
        , storage_{ other.storage_ } {}

    Delegate(Delegate&& other) noexcept
        : function_{ std::move(other.function_) }
        , storage_{ std::move(other.storage_) } {}

    Delegate& operator=(const Delegate& other) {
        function_ = other.function_;
        storage_ = other.storage_;
        return *this;
    }

    Delegate& operator=(Delegate&& other) noexcept {
        function_ = std::move(other.function_);
        storage_ = std::move(other.storage_);
        return *this;
    }

    Delegate& operator=(nullptr_t) noexcept {
        function_ = nullptr;
        storage_ = nullptr;
        return *this;
    }

    UGINE_FORCE_INLINE operator bool() const { return function_ != nullptr; }

    template <typename Invocable> Delegate(Invocable invocable) { Connect<Invocable>(invocable); }

    template <auto Candidate> void Connect() noexcept {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Args...>);

        new (&storage_) void* { nullptr };

        function_ = +[](Storage&, Args... args) { return std::invoke(Candidate, args...); };
    }

    template <auto Candidate, class Inst> void Connect(Inst inst) {
        static_assert(sizeof(Inst) <= StorageSize);
        static_assert(std::is_trivially_copyable_v<Inst> && std::is_trivially_destructible_v<Inst>);
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Inst&, Args...>);

        new (&storage_) Inst{ inst };

        function_ = +[](Storage& storage, Args... args) {
            Inst inst{ *reinterpret_cast<Inst*>(&storage) };
            return std::invoke(Candidate, inst, args...);
        };
    }

    template <typename Invocable> void Connect(Invocable invocable) {
        static_assert(sizeof(Invocable) <= StorageSize);
        static_assert(std::is_invocable_r_v<Ret, Invocable, Args...>);
        static_assert(std::is_trivially_destructible_v<Invocable>);
        static_assert(std::is_class_v<Invocable>);

        new (&storage_) Invocable{ std::move(invocable) };

        function_ = +[](Storage& storage, Args... args) {
            Invocable& inv{ *reinterpret_cast<Invocable*>(&storage) };
            return std::invoke(inv, args...);
        };
    }

    Ret operator()(Args... args) const {
        UGINE_ASSERT(function_);
        return function_(storage_, args...);
    }

private:
    Signature* function_{};
    mutable alignas(StorageSize) Storage storage_{};
};

} // namespace ugine