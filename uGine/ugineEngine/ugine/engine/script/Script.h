#pragma once

namespace ugine {

class Engine;

class Script {
public:
    virtual ~Script() = default;

    virtual const char* Name() const = 0;
    virtual void OnAttach() {}
    virtual void OnDetach() {}

    virtual void EarlyUpdate(Engine& engine) {}
    virtual void Update(Engine& engine) {}
    virtual void LateUpdate(Engine& engine) {}
};

} // namespace ugine