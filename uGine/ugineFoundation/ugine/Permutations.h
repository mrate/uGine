#pragma once

#include <ugine/Ugine.h>
#include <ugine/Vector.h>

namespace ugine {

template <typename T> class Permutations {
public:
    Permutations(Span<const T> variants)
        : variants_{ variants }
        , count_{ u32(1) << variants.Size() } {}

    Vector<T> Next() {
        Vector<T> result;

        u32 i{};
        for (u32 index{ index_ }; index; index >>= 1) {
            if (index & 1) {
                result.PushBack(variants_[i]);
            }
            ++i;
        }

        ++index_;
        return result;
    }

    bool End() const { return index_ >= count_; }

private:
    Span<const T> variants_;
    u32 index_{};
    u32 count_{};
};

} // namespace ugine