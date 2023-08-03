#pragma once

#include "LuaScript.h"
#include "NativeScript.h"

#include <memory>
#include <vector>

namespace ugine {

struct NativeScriptComponent {
    void Add(const GameObject& go, const std::shared_ptr<NativeScript>& script) {
        scripts_.push_back(script);
        script->OnAttach(go);
    }

    void Remove(const GameObject& go, const std::shared_ptr<NativeScript>& script) {
        scripts_.erase(std::remove(scripts_.begin(), scripts_.end(), script), scripts_.end());
        script->OnDetach(go);
    }

    const std::vector<std::shared_ptr<NativeScript>>& Scripts() const { return scripts_; }

private:
    std::vector<std::shared_ptr<NativeScript>> scripts_;
};

struct LuaScriptComponent {
    ResourceHandle<LuaScript> script;
};

} // namespace ugine