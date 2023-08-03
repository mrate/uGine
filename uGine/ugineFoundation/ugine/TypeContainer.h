#pragma once

namespace ugine {

template <typename... T> struct TypeContainer {
    static constexpr int Size = 0;

    template <typename V> static constexpr int IndexOf() { static_assert(false, "Type not found in TypeContainer"); }
};

template <typename T, typename... H> struct TypeContainer<T, H...> : TypeContainer<H...> {
    using Type = T;
    using Rest = TypeContainer<H...>;
    static constexpr int Size = 1 + TypeContainer<H...>::Size;

    template <typename V> static constexpr int IndexOf() { return 1 + Rest::template IndexOf<V>(); }

    template <> static constexpr int IndexOf<T>() { return 0; }
};

} // namespace ugine
