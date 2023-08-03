#pragma once

#include "Params.h"

#include <ugine/engine/system/Event.h>

namespace ugine {

class Engine;

class System {
public:
    explicit System(Engine& engine)
        : engine_{ engine } {}
    virtual ~System() = default;

    virtual void Initialize() {}

    virtual bool HandleSystemEvent(const SystemEvent& event) { return false; }

    virtual void EarlyUpdate() {}
    virtual void Update() {}
    virtual void LateUpdate() {}

    virtual void Sync() {}

protected:
    Engine& GetEngine() { return engine_; }

private:
    Engine& engine_;
};

} // namespace ugine