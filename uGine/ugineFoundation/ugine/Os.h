#pragma once

#include <ugine/Span.h>
#include <ugine/String.h>

namespace ugine {

bool RunProcess(StringView process, Span<String> arguments);

}