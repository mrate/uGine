#include "CVars.h"

namespace ugine {

CVar& CVars::Get(StringID hash) {
    static CVar nullVal;

    const auto it{ vars_.find(hash) };
    return it != vars_.end() ? it->second : nullVal;
}

} // namespace ugine