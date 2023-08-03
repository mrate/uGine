#pragma once

#include <ugine/UUID.h>
#include <ugine/Ugine.h>

namespace ugine {

struct ResourceID {
    static ResourceID Generate() { return ResourceID{ .uuid = UUID::Generate() }; }
    String ToString() const { return uuid.ToString(); }
    bool IsNull() const { return uuid.IsNull(); }

    UUID uuid{};
};

UGINE_FORCE_INLINE bool operator==(const ResourceID& a, const ResourceID& b) {
    return a.uuid == b.uuid;
}

UGINE_FORCE_INLINE void serialize(auto& s, ResourceID& o) {
    serialize(s, o.uuid);
}

} // namespace ugine