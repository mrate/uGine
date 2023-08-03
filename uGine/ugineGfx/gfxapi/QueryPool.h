#pragma once

#include <ugine/Ugine.h>

namespace ugine::gfxapi {

class QueryPool {
public:
    virtual ~QueryPool() = default;

    virtual void FetchResults() = 0;

    [[nodiscard]] virtual u32 Allocate() = 0;
    [[nodiscard]] virtual u64 Result(u32 query) const = 0;
    [[nodiscard]] virtual u32 Count() const = 0;
    [[nodiscard]] virtual double GetMicroseconds(u64 timestamp) const = 0;
};

} // namespace ugine::gfxapi