#include "CoreSystem.h"

namespace ugine::core {

CoreSystem::CoreSystem(Engine& engine)
    : System{ engine } {
}

CoreSystem::~CoreSystem() {
}

void CoreSystem::EarlyUpdate() {
}

} // namespace ugine::core