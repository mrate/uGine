#pragma once

#include <ugine/Hash.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>

#include <array>

namespace ugine {
struct UUID {
    static UUID Generate();

    UUID() { v.fill(0); }

    UGINE_FORCE_INLINE bool IsNull() const { return v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0; }

    static UUID FromString(StringView str) {
        if (str.Size() != 35) {
            return UUID{};
        }

        UUID res{};
        u32 val{};
        int pos{};
        for (auto c : str) {
            if (c >= '0' && c <= '9') {
                val = (val << 4) | (c - '0');
            } else if (c >= 'a' && c <= 'f') {
                val = (val << 4) | (10 + (c - 'a'));
            } else if (c == '-') {
                res.v[pos++] = val;
                val = 0;
            } else {
                return UUID{}; // Parse error.
            }

            if (pos >= res.v.size()) {
                break;
            }
        }

        if (pos < res.v.size()) {
            res.v[pos++] = val;
        }

        return res;
    }

    String ToString() const {
        static const char hex[] = "0123456789abcdef";

        String result{ "xxxxxxxx-xxxxxxxx-xxxxxxxx-xxxxxxxx" };
        for (int i{}; i < 4; ++i) {
            auto val{ v[i] };
            for (int j{}; j < 8; ++j) {
                result[i * (8 + 1) + 8 - 1 - j] = hex[val & 0xf];
                val >>= 4;
            }
        }

        return result;
    }

    std::array<u32, 4> v;
};

inline bool operator==(const UUID& a, const UUID& b) {
    for (int i{}; i < 4; ++i) {
        if (a.v[i] != b.v[i]) {
            return false;
        }
    }
    return true;
}

void serialize(auto& s, UUID& v) {
    for (int i{}; i < 4; ++i) {
        s.value4b(v.v[i]);
    }
}
} // namespace ugine

namespace std {
template <> struct hash<ugine::UUID> {
    size_t operator()(const ugine::UUID& v) const noexcept {
        size_t res{ 0 };
        ugine::HashCombine(res, v.v[0], v.v[1], v.v[2], v.v[3]);
        return res;
    }
};
} // namespace std