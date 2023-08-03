#pragma once

#include <ugine/Json.h>
#include <ugine/engine/core/Resource.h>

namespace nlohmann {

template <> struct adl_serializer<ugine::ResourceID> {
    static void to_json(json& j, const ugine::ResourceID& v) { j["uuid"] = v.uuid; }
    static void from_json(const json& j, ugine::ResourceID& v) { v.uuid = j["uuid"].get<ugine::UUID>(); }
};

template <typename T> struct adl_serializer<ugine::ResourceHandle<T>> {
    static void to_json(json& j, const ugine::ResourceHandle<T>& v) { j["id"] = v.Get() ? v.Get()->Id() : ugine::ResourceID{}; }
    static void from_json(const json& j, ugine::ResourceHandle<T>& v) {
        const auto id{ j["id"].get<ugine::ResourceID>() };
        UGINE_ASSERT(false);
    }
};

} // namespace nlohmann
