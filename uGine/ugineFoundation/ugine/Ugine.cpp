#include "Ugine.h"

#include <Windows.h>

namespace ugine {

void Break() {
    if (IsDebuggerPresent()) {
        DebugBreak();
    }
}

} // namespace ugine
