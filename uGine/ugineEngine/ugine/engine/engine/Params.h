#pragma once

#include <ugine/ugineConfig.h>

#include <ugine/String.h>
#include <ugine/Ugine.h>
#include <ugine/Utils.h>
#include <ugine/Path.h>

namespace ugine {

enum class Systems : u8 {
    Core = UGINE_BIT(0),
    Graphics = UGINE_BIT(1),
    Input = UGINE_BIT(2),
    Script = UGINE_BIT(3),

    All = 0xff,
};

UGINE_FLAGS(Systems, u8);

struct Version {
    int major{};
    int minor{};
    int file{};
};

struct EngineParams {
    String appName{};
    Version appVersion{};

    Systems systems{ Systems::All };

    bool fullscreen{};
    u32 width{};
    u32 height{};
    bool srgb{};
    bool debugGraphics{};

    bool imgui{};
    Path rootPath{};

    // TODO:
    // custom hwnd / android surface
    // headless
    // etc.
};

} // namespace ugine