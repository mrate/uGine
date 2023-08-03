#pragma once

#include <ugine/Json.h>
#include <ugine/String.h>
#include <ugine/Path.h>

namespace ugine::ed {

class EditorSettings {
public:
    template <typename T> void Set(StringView name, T value) {
        auto& json{ GetPath(name) };
        json = value;
    }

    template <typename T> T Get(StringView name, T defaultValue = T{}) {
        auto& json{ GetPath(name) };
        try {
            return json.get<T>();
        } catch (...) {
            return defaultValue;
        }
    }

    void Load(const Path& path);
    void Save(const Path& path);

private:
    nlohmann::json& GetPath(StringView name);

    nlohmann::json json_{};
};

} // namespace ugine::ed