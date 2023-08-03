#pragma once

#include <ugine/engine/engine/System.h>

namespace ugine::system {
class Platform;
}

namespace ugine::core {

class CoreSystem final : public System {
public:
    explicit CoreSystem(Engine& engine);
    ~CoreSystem();

    void EarlyUpdate();
};

} // namespace ugine::core