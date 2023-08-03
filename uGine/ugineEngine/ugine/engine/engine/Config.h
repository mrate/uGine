#pragma once

#include <ugine/String.h>
#include <ugine/Ugine.h>

namespace ugine {

class Configuration {
public:
    template <typename T> T GetParam(StringID name, T defaultVal) const {
        // TODO:
        return defaultVal;
    }

    void Load(StringView path) {
        // TODO:
    }
};

} // namespace ugine