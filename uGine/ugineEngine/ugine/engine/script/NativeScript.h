#pragma once

#include "Script.h"

#include <ugine/engine/world/World.h>

namespace ugine {

class NativeScript : public Script {
public:
    virtual ~NativeScript() {}

    virtual const char* Name() const = 0;
    virtual void OnAttach() {}
    virtual void OnDetach() {}

    virtual void EarlyUpdate(Engine& engine) {}
    virtual void Update(Engine& engine) {}
    virtual void LateUpdate(Engine& engine) {}

    GameObject& Owner() { return owner_; }

    const GameObject& Owner() const { return owner_; }

    World& World() {
        auto scene{ Owner().World() };
        UGINE_ASSERT(scene);
        return *scene;
    }

    template <typename T> T* Component() {
        UGINE_ASSERT(Owner());
        return Owner()->template Component<T>();
    }

    Transformation LocalTransformation() const;
    void SetLocalTransformation(const ugine::Transformation& trans);
    void LookAt(const glm::vec3& to);
    void LookAt(const GameObject& go);
    void OnAttach(const GameObject& go) {
        if (owner_ != go) {
            owner_ = go;
            OnAttach();
        }
    }
    void OnDetach(const GameObject& go) {
        OnDetach();
        owner_ = {};
    }

private:
    GameObject owner_{};
};

} // namespace ugine
