#pragma once

#include "../EditorTool.h"

#include "../widgets/ImGuiScope.h"

#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/script/Component.h>
#include <ugine/engine/world/Component.h>

#include <ugine/engine/world/GameObject.h>

#include <ugine/EventEmittor.h>

//#include "PostprocessEditor.h"

namespace ugine::ed {

class EditorContext;

class GameObjectEditor : public EditorTool {
public:
    explicit GameObjectEditor(EditorContext& context);

    void Populate(GameObject& go, bool open = true) const;

    void Render() override;

private:
    template <typename T> void RenderComponent(GameObject& go, const char* label, int flags) const {
        if (go.Has<T>()) {
            auto visible{ true };
            if (ImGui::CollapsingHeader(label, &visible, flags)) {
                ImScope::Indent i;
                Populate(go, go.Component<T>());
            }
            if (!visible) {
                go.RemoveComponent<T>();
            }
        }
    }

    void Populate(GameObject& go, const TagComponent& tag) const;
    void Populate(GameObject& go, const TransformationComponent& trans) const;
    void Populate(GameObject& go, const CameraComponent& camera) const;
    void Populate(GameObject& go, const NativeScriptComponent& script) const;
    void Populate(GameObject& go, const LuaScriptComponent& script) const;
    void Populate(GameObject& go, const MeshComponent& mesh) const;
    void Populate(GameObject& go, const LightComponent& light) const;
    void Populate(GameObject& go, const AnimationControllerComponent& animator) const;
    void Populate(GameObject& go, const SkyComponent& sky) const;
};

} // namespace ugine::ed