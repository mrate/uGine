#pragma once

#include <stdint.h>

namespace ugine::tools {

struct EmbeddedFile {
    const char* name{};
    const char* data{};
    const size_t size{};
};

} // namespace ugine::tools
